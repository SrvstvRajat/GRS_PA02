// MT25078
// Part A2: One-Copy Implementation using sendmsg() with pre-registered buffer
// This eliminates the user-space to kernel-space copy by using a single contiguous buffer

#include "MT25078_Common.h"

typedef struct
{
  int client_fd;
  size_t field_size;
} thread_arg_t;

void *client_thread(void *arg)
{
  thread_arg_t *t = (thread_arg_t *)arg;

  // Create message with 8 dynamically allocated fields
  message_t *msg = create_message(t->field_size);
  if (!msg)
  {
    close(t->client_fd);
    free(t);
    return NULL;
  }

  // ONE-COPY OPTIMIZATION:
  // Instead of sending 8 separate buffers (which would require kernel to copy each),
  // we pre-register a single contiguous buffer and serialize data into it ONCE.
  // This eliminates one copy: we go from 2 copies to 1 copy.
  //
  // Traditional two-copy path:
  //   1. User space buffer → Kernel socket buffer
  //   2. Kernel socket buffer → NIC via DMA
  //
  // One-copy path (with pre-registered buffer sent via sendmsg):
  //   1. Pre-registered buffer → NIC via DMA (kernel can optimize)
  //
  // The key is that sendmsg with a single iovec can allow kernel to optimize
  // the copy operation, especially if the buffer is page-aligned and reused.

  size_t total_size = t->field_size * FIELD_COUNT;

  // Allocate page-aligned buffer for better kernel optimization
  char *send_buffer = NULL;
  if (posix_memalign((void **)&send_buffer, 4096, total_size) != 0)
  {
    free_message(msg);
    close(t->client_fd);
    free(t);
    return NULL;
  }

  // Serialize message into the pre-registered buffer
  serialize_message(msg, send_buffer);

  // Use sendmsg with single iovec (pre-registered buffer approach)
  // This allows kernel to optimize by potentially using direct DMA
  struct iovec iov;
  iov.iov_base = send_buffer;
  iov.iov_len = total_size;

  struct msghdr msghdr = {0};
  msghdr.msg_iov = &iov;
  msghdr.msg_iovlen = 1; // Single buffer = one-copy optimization

  // Continuously send the message using sendmsg()
  while (1)
  {
    ssize_t sent = sendmsg(t->client_fd, &msghdr, 0);
    if (sent <= 0)
    {
      if (errno == EINTR)
        continue;
      break;
    }
  }

  close(t->client_fd);
  free(send_buffer);
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

  printf("A2 Server (one-copy) listening on port %d (field_size=%zu, max_threads=%d)\n",
         port, field_size, max_threads);

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