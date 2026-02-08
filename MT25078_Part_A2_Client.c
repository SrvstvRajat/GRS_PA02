#include "MT25078_Common.h"

int main(int argc, char *argv[])
{
  if (argc < 5)
  {
    fprintf(stderr, "Usage: %s <ip> <port> <field_size> <duration_sec>\n", argv[0]);
    return 1;
  }

  char *ip = argv[1];
  int port = atoi(argv[2]);
  size_t field_size = atoi(argv[3]);
  int duration = atoi(argv[4]);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    perror("socket");
    return 1;
  }

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = htons(port)};

  if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
  {
    perror("inet_pton");
    return 1;
  }

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("connect");
    return 1;
  }

  // Message is 8 fields
  size_t msg_size = field_size * FIELD_COUNT;
  char *buffer = malloc(msg_size);
  if (!buffer)
  {
    perror("malloc");
    return 1;
  }

  // Use recvmsg with iovec
  struct iovec iov = {buffer, msg_size};
  struct msghdr msghdr = {0};
  msghdr.msg_iov = &iov;
  msghdr.msg_iovlen = 1;

  // Variables for measurement
  size_t total_bytes = 0;
  size_t message_count = 0;
  double total_latency_ms = 0;

  double start_time = get_time_ms();
  double end_time = start_time + (duration * 1000.0);

  while (get_time_ms() < end_time)
  {
    double msg_start = get_time_ms();
    
    ssize_t received = recvmsg(sock, &msghdr, 0);
    
    double msg_end = get_time_ms();

    if (received <= 0)
    {
      if (errno == EINTR)
        continue;
      break;
    }

    total_bytes += received;
    message_count++;
    total_latency_ms += (msg_end - msg_start);
  }

  double actual_duration_ms = get_time_ms() - start_time;
  double actual_duration_sec = actual_duration_ms / 1000.0;

  // Calculate metrics
  double throughput_gbps = (total_bytes * 8.0) / (actual_duration_sec * 1e9);
  double avg_latency_us = (total_latency_ms * 1000.0) / message_count;

  // Print results
  printf("RESULTS: bytes=%zu messages=%zu duration_ms=%.2f throughput_gbps=%.6f latency_us=%.2f\n",
         total_bytes, message_count, actual_duration_ms, throughput_gbps, avg_latency_us);

  close(sock);
  free(buffer);
  return 0;
}