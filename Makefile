CC = gcc
CFLAGS = -Wall -Wextra -O3
LIBS = -lpaho-mqtt3c
TARGET = publisher
SRCS = publisher.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
