CC = g++
CFLAGS = -Wall
LIBS = -lsqlite3 -lmemcached
TARGET = slave_node
SRCS = main.cpp mongoose.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LIBS)

clean:
	rm -f $(TARGET)
