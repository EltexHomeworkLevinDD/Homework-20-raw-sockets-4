#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <errno.h>

#include "udp_client_module.h"

int main() {
    const char *ifname = "enp0s8";
    const char *src_ip = "192.168.0.10";
    const unsigned short src_port = 4444;
    const char *dest_ip = "192.168.0.11";
    const unsigned short dest_port = 55555;
    const unsigned char dest_mac[6] = {0x08, 0x00, 0x27, 0x28, 0x0c, 0x60};
    const char *payload = "hello";
    int payload_len = strlen(payload);

    if (send_raw_packet(ifname, dest_mac, src_ip, dest_ip, src_port, dest_port,
                payload, payload_len) < 0) {
        fprintf(stderr, "Failed to send raw packet\n");
        return 1;
    }

    printf("Packet sent successfully\n");

    char recv_buffer[ETH_FRAME_LEN];
    int recv_len = receive_raw_packet(ifname, recv_buffer, ETH_FRAME_LEN,
                dest_port);
    if (recv_len < 0) {
        fprintf(stderr, "Failed to receive raw packet\n");
        return 1;
    }

    printf("Received response: %s\n", recv_buffer + sizeof(struct ether_header)+
                sizeof(struct iphdr) + sizeof(struct udphdr));

    return 0;
}
