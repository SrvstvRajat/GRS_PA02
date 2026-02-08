# MT25078
CC = gcc
CFLAGS = -Wall -pthread -O2
LDFLAGS = -pthread

# Object files
COMMON_OBJ = MT25078_Common.o

# Executables
A1_SERVER = A1_server
A1_CLIENT = A1_client
A2_SERVER = A2_server
A2_CLIENT = A2_client
A3_SERVER = A3_server
A3_CLIENT = A3_client

ALL_TARGETS = $(A1_SERVER) $(A1_CLIENT) $(A2_SERVER) $(A2_CLIENT) $(A3_SERVER) $(A3_CLIENT)

.PHONY: all clean

all: $(ALL_TARGETS)

# Common object file
MT25078_Common.o: MT25078_Common.c MT25078_Common.h
	$(CC) $(CFLAGS) -c MT25078_Common.c -o MT25078_Common.o

# A1 (Two-Copy) targets
$(A1_SERVER): MT25078_Part_A1_Server.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) MT25078_Part_A1_Server.c $(COMMON_OBJ) -o $(A1_SERVER) $(LDFLAGS)

$(A1_CLIENT): MT25078_Part_A1_Client.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) MT25078_Part_A1_Client.c $(COMMON_OBJ) -o $(A1_CLIENT) $(LDFLAGS)

# A2 (One-Copy) targets
$(A2_SERVER): MT25078_Part_A2_Server.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) MT25078_Part_A2_Server.c $(COMMON_OBJ) -o $(A2_SERVER) $(LDFLAGS)

$(A2_CLIENT): MT25078_Part_A2_Client.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) MT25078_Part_A2_Client.c $(COMMON_OBJ) -o $(A2_CLIENT) $(LDFLAGS)

# A3 (Zero-Copy) targets
$(A3_SERVER): MT25078_Part_A3_Server.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) MT25078_Part_A3_Server.c $(COMMON_OBJ) -o $(A3_SERVER) $(LDFLAGS)

$(A3_CLIENT): MT25078_Part_A3_Client.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) MT25078_Part_A3_Client.c $(COMMON_OBJ) -o $(A3_CLIENT) $(LDFLAGS)

clean:
	rm -f $(ALL_TARGETS) *.o perf*.txt client*.txt *.log *.txt