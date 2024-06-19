CC = gcc
CFLAGS = -pthread
TARGET = client

all: $(TARGET)

$(TARGET): client.c
	$(CC) $(CFLAGS) -o $(TARGET) client.c

clean:
	rm -f $(TARGET) *.o
