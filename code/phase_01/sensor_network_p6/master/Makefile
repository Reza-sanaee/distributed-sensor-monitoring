CC = g++
CFLAGS = -Wall

LIBS_MASTER = -lsqlite3 -lmemcached -lmosquitto -pthread
LIBS_DAEMON = -lsqlite3

TARGET_MASTER = master_node
TARGET_DAEMON = alert_daemon

SRCS_MASTER = main.cpp mongoose.c
SRCS_DAEMON = alert_daemon.cpp

all: $(TARGET_MASTER) $(TARGET_DAEMON)

$(TARGET_MASTER): $(SRCS_MASTER)
	$(CC) $(CFLAGS) -o $(TARGET_MASTER) $(SRCS_MASTER) $(LIBS_MASTER)

$(TARGET_DAEMON): $(SRCS_DAEMON)
	$(CC) $(CFLAGS) -o $(TARGET_DAEMON) $(SRCS_DAEMON) $(LIBS_DAEMON)

clean:
	rm -f $(TARGET_MASTER) $(TARGET_DAEMON)
