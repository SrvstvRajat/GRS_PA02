#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>

#define FIELD_COUNT 8
#define DEFAULT_PORT 9090
#define BACKLOG 128

// Message structure with 8 dynamically allocated string fields
typedef struct
{
  char *fields[FIELD_COUNT];
  size_t field_size;
} message_t;

// Function prototypes
message_t *create_message(size_t field_size);
void free_message(message_t *msg);
size_t serialize_message(message_t *msg, char *buffer);
message_t *deserialize_message(char *buffer, size_t field_size);

// Timing utilities
double get_time_ms();

#endif