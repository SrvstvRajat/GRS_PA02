#include "MT25078_Common.h"

message_t *create_message(size_t field_size)
{
  message_t *msg = malloc(sizeof(message_t));
  if (!msg)
    return NULL;

  msg->field_size = field_size;

  for (int i = 0; i < FIELD_COUNT; i++)
  {
    msg->fields[i] = malloc(field_size);
    if (!msg->fields[i])
    {
      // Cleanup on failure
      for (int j = 0; j < i; j++)
        free(msg->fields[j]);
      free(msg);
      return NULL;
    }
    memset(msg->fields[i], 'A' + i, field_size);
  }

  return msg;
}

void free_message(message_t *msg)
{
  if (!msg)
    return;

  for (int i = 0; i < FIELD_COUNT; i++)
  {
    if (msg->fields[i])
      free(msg->fields[i]);
  }

  free(msg);
}

// Serialize message to a flat buffer for sending
size_t serialize_message(message_t *msg, char *buffer)
{
  size_t offset = 0;
  for (int i = 0; i < FIELD_COUNT; i++)
  {
    memcpy(buffer + offset, msg->fields[i], msg->field_size);
    offset += msg->field_size;
  }
  return offset;
}

// Deserialize buffer back to message structure
message_t *deserialize_message(char *buffer, size_t field_size)
{
  message_t *msg = create_message(field_size);
  if (!msg)
    return NULL;

  size_t offset = 0;
  for (int i = 0; i < FIELD_COUNT; i++)
  {
    memcpy(msg->fields[i], buffer + offset, field_size);
    offset += field_size;
  }

  return msg;
}

// Get current time in milliseconds
double get_time_ms()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}