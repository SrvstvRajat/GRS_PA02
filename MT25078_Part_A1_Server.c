#include "MT25078_Common.h"

typedef struct
{
  int client_fd;
  size_t field_size; // Size per field
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

  // Serialize message to flat buffer for sending
  size_t total_size = t->field_size * FIELD_COUNT;
  char *send_buffer = malloc(total_size);
  if (!send_buffer)
  {
    free_message(msg);
    close(t->client_fd);
    free(t);
    return NULL;
  }

  serialize_message(msg, send_buffer);

  // Continuously send the message using send()
  while (1)
  {
    ssize_t sent = send(t->client_fd, send_buffer, total_size, 0);
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

  printf("Server listening on port %d (field_size=%zu, max_threads=%d)\n",
         port, field_size, max_threads);

  int thread_count = 0;

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

    // Limit concurrent clients based on max_threads
    if (thread_count >= max_threads)
    {
      fprintf(stderr, "Max threads reached, rejecting connection\n");
      close(client_fd);
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
    thread_count++;
  }

  close(server_fd);
  return 0;
}
