#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP_S "127.0.0.1"
#define SERVER_PORT_H 12345
#define BUFFER_SIZE 1024

void to_uppercase(char *str) {
    while (*str) {
        *str = toupper((unsigned char) *str);
        str++;
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_addr_len = sizeof(client_addr);
    ssize_t recv_len;

    // Создание сокета
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // Задание адреса и порта сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP_S, (void*)&(server_addr.sin_addr));
    server_addr.sin_port = htons(SERVER_PORT_H);

    // Привязка сокета к адресу и порту
    if (bind(sockfd, (const struct sockaddr *)&server_addr, 
            sizeof(server_addr)) < 0) {
        perror("bind()");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening...\n");

    // Получение и отправка сообщений
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)
                &client_addr, &client_addr_len);
        if (recv_len < 0) {
            perror("recvfrom()");
            continue;
        }

        printf("Received packet from %s:%d\n", inet_ntoa(client_addr.sin_addr), 
                ntohs(client_addr.sin_port));

        to_uppercase(buffer);

        if (sendto(sockfd, buffer, recv_len, 0, (struct sockaddr *)&client_addr, 
                client_addr_len) < 0) {
            perror("sendto()");
        }

        printf("Resended to %s:%d\n", inet_ntoa(client_addr.sin_addr), 
                ntohs(client_addr.sin_port));
    }

    close(sockfd);
    return 0;
}
