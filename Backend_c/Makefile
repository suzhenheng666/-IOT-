TARGET  = iot_backend
SRCS    = main.c store.c mqtt_client.c http_server.c cJSON.c
OBJS    = $(SRCS:.c=.o)

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lmosquitto -lmicrohttpd -lpthread -lm

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "=== 编译完成: ./$(TARGET) ==="

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

deps:
	@echo "安装系统依赖:"
	@echo "  sudo apt install libmosquitto-dev libmicrohttpd-dev"
