CFLAGS := -Wall -Wextra -g

.PHONY: all clean

all: udp_server udp_client

udp_server: udp_server.c
	gcc $(CFLAGS) $^ -o $@

udp_client_module.o: udp_client_module.c udp_client_module.h
	gcc $(CFLAGS) $< -c

udp_client.o: udp_client.c udp_client_module.h
	gcc $(CFLAGS) $< -c

udp_client: udp_client.o udp_client_module.o
	gcc $(CFLAGS) $^ -o $@

clean:
	rm -rf *.o
	rm -rf udp_server
	rm -rf udp_client