// MT25078
// Part A3: Zero-Copy Implementation using MSG_ZEROCOPY
// This eliminates both copies by using DMA directly from user buffer to NIC

#include "MT25078_Common.h"
#include <linux/errqueue.h>
#include <poll.h>

#ifndef MSG_ZEROCOPY
#define MSG_ZEROCOPY 0x4000000
#endif

#ifndef SO_ZEROCOPY
#define SO_ZEROCOPY 60
#endif

#ifndef MSG_ERRQUEUE
#define MSG_ERRQUEUE 0x2000
#endif

typedef struct
{
  int client_fd;
  size_t field_size;
} thread_arg_t;

// Function to handle zero-copy completion notifications
// This checks the socket error queue for send completion
int handle_zerocopy_completions(int fd, int flags)
{
  struct sock_extended_err *serr;
  struct msghdr msg = {};
  struct cmsghdr *cmsg;
  uint32_t hi, lo;
  char cbuf[100];

  msg.msg_control = cbuf;
  msg.msg_controllen = sizeof(cbuf);

  int ret = recvmsg(fd, &msg, MSG_ERRQUEUE | flags);
  if (ret < 0)
  {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return 0; // No completions available
    return -1;
  }

  cmsg = CMSG_FIRSTHDR(&msg);
  if (!cmsg || cmsg->cmsg_level != SOL_IP || cmsg->cmsg_type != IP_RECVERR)
    return -1;

  serr = (struct sock_extended_err *)CMSG_DATA(cmsg);

  if (serr->ee_errno != 0 || serr->ee_origin != SO_EE_ORIGIN_ZEROCOPY)
    return -1;

  // Extract the range of completed send operations
  hi = serr->ee_data;
  lo = serr->ee_info;

  return 0; // Success
}

void *client_thread(void *arg)
{
  thread_arg_t *t = (thread_arg_t *)arg;

  // Enable zero-copy on socket
  int enable = 1;
  if (setsockopt(t->client_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable)) < 0)
  {
    perror("setsockopt SO_ZEROCOPY");
    // Continue anyway - may not be supported on all systems
  }

  // Set socket to non-blocking for error queue handling
  int flags = fcntl(t->client_fd, F_GETFL, 0);
  fcntl(t->client_fd, F_SETFL, flags | O_NONBLOCK);

  // Create message with 8 dynamically allocated fields
  message_t *msg = create_message(t->field_size);
  if (!msg)
  {
    close(t->client_fd);
    free(t);
    return NULL;
  }

  // ZERO-COPY OPTIMIZATION:
  // MSG_ZEROCOPY allows the kernel to DMA directly from user buffer to NIC.
  // Traditional path:
  //   1. User buffer → Kernel socket buffer (copy 1)
  //   2. Kernel buffer → NIC via DMA (copy 2)
  //
  // Zero-copy path:
  //   User buffer → NIC via DMA (0 copies - kernel pins pages and DMAs directly)
  //
  // Important: Buffer must remain valid until completion notification received

  // Use iovec for zero-copy sending
  struct iovec iov[FIELD_COUNT];
  for (int i = 0; i < FIELD_COUNT; i++)
  {
    iov[i].iov_base = msg->fields[i];
    iov[i].iov_len = t->field_size;
  }

  struct msghdr msghdr = {0};
  msghdr.msg_iov = iov;
  msghdr.msg_iovlen = FIELD_COUNT;

  uint32_t send_count = 0;
  uint32_t completed_count = 0;

  // Continuously send with MSG_ZEROCOPY flag
  while (1)
  {
    ssize_t sent = sendmsg(t->client_fd, &msghdr, MSG_ZEROCOPY);
    if (sent <= 0)
    {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
      {
        // Check for completions
        handle_zerocopy_completions(t->client_fd, MSG_DONTWAIT);
        continue;
      }
      break;
    }

    send_count++;

    // Periodically check for completion notifications
    // This prevents the error queue from overflowing
    if (send_count % 1000 == 0)
    {
      handle_zerocopy_completions(t->client_fd, MSG_DONTWAIT);
    }
  }

  // Drain any remaining completion notifications
  while (handle_zerocopy_completions(t->client_fd, MSG_DONTWAIT) == 0)
  {
    // Keep draining
  }

  close(t->client_fd);
  free_message(msg);
  free(t);
  return NULL;
}

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    fprintf(stderr, "Usage: %s <port> <field_size> <max_threads>\n", argv[0]);
    return 1;
  }

  int port = atoi(argv[1]);
  size_t field_size = atoi(argv[2]);
  int max_threads = atoi(argv[3]);

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    perror("socket");
    return 1;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr.s_addr = INADDR_ANY};

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    return 1;
  }

  if (listen(server_fd, BACKLOG) < 0)
  {
    perror("listen");
    return 1;
  }

  printf("A3 Server (zero-copy) listening on port %d (field_size=%zu, max_threads=%d)\n",
         port, field_size, max_threads);
  printf("Note: MSG_ZEROCOPY requires Linux kernel 4.14+\n");

  while (1)
  {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
    if (client_fd < 0)
    {
      perror("accept");
      continue;
    }

    pthread_t tid;
    thread_arg_t *t = malloc(sizeof(thread_arg_t));
    t->client_fd = client_fd;
    t->field_size = field_size;

    if (pthread_create(&tid, NULL, client_thread, t) != 0)
    {
      perror("pthread_create");
      free(t);
      close(client_fd);
      continue;
    }

    pthread_detach(tid);
  }

  close(server_fd);
  return 0;
}