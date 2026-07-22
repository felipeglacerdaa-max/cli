CC=gcc
CFLAGS=-O2 -Wall -Wextra
LIBS=-lcurl
SRC=$(wildcard src/*.c)
TARGET=tiktok-dl

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
