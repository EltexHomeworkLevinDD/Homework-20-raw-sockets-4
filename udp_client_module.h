#ifndef UDP_CLIENT_MODULE_H
#define UDP_CLIENT_MODULE_H

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

unsigned short checksum(void *buf);
int get_mac_address(const char *ifname, unsigned char *mac);
int send_raw_packet(const char *ifname, const unsigned char *dest_mac,
            const char *src_ip, const char *dest_ip, unsigned short src_port,
            unsigned short dest_port, const char *payload, int payload_len);
int receive_raw_packet(const char *ifname, char *buffer, int buf_len,
            unsigned short expected_dest_port);

#endif
