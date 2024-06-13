#include "udp_client_module.h"

/**
 * @brief Вычисление контрольной суммы IP-заголовка
 * 
 * @param buf Указатель на буфер, содержащий IP-заголовок
 * @return unsigned short Контрольная сумма
 */
unsigned short checksum(void *buf) {
    unsigned long sum = 0;
    unsigned short *ptr = buf;

    for (int i = 0; i < 10; i++) {
        sum += *ptr++;
    }

    unsigned short tmp = sum >> 16;
    sum = (sum & 0xFFFF) + tmp;
    sum = ~sum;
    return sum;
}

/**
 * @brief Получение MAC-адреса интерфейса
 * 
 * @param ifname Имя интерфейса
 * @param mac Указатель на буфер для хранения MAC-адреса
 * @return int 0 при успехе, -1 при ошибке
 */
int get_mac_address(const char *ifname, unsigned char *mac) {
    struct ifaddrs *ifaddr, *ifa;

    // Получение списка сетевых интерфейсов
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    // Поиск нужного интерфейса
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) 
            continue;

        // Проверка, что интерфейс является AF_PACKET и совпадает с именем
        if ((ifa->ifa_addr->sa_family == AF_PACKET) && 
                strcmp(ifa->ifa_name, ifname) == 0) {
            struct sockaddr_ll *s = (struct sockaddr_ll *)ifa->ifa_addr;
            memcpy(mac, s->sll_addr, 6);
            freeifaddrs(ifaddr);
            return 0;
        }
    }

    freeifaddrs(ifaddr);
    return -1;
}

/**
 * @brief Создание и отправка RAW пакета
 * 
 * @param ifname Имя интерфейса
 * @param dest_mac MAC-адрес получателя
 * @param src_ip IP-адрес отправителя
 * @param dest_ip IP-адрес получателя
 * @param src_port UDP порт отправителя
 * @param dest_port UDP порт получателя
 * @param payload Данные полезной нагрузки
 * @param payload_len Длина полезной нагрузки
 * @return int 0 при успехе, -1 при ошибке
 */
int send_raw_packet(const char *ifname, const unsigned char *dest_mac, 
            const char *src_ip, const char *dest_ip, unsigned short src_port, 
            unsigned short dest_port, const char *payload, int payload_len) {
    int sockfd;
    unsigned char buffer[ETH_FRAME_LEN]; // Буфер для хранения Ethernet фрейма
    struct ether_header *eth_header = 
                (struct ether_header *)buffer; // Заголовок Ethernet
    struct iphdr *ip_header = (struct iphdr *)
                (buffer + sizeof(struct ether_header)); // Заголовок IP
    struct udphdr *udp_header = (struct udphdr *)(buffer + 
                sizeof(struct ether_header) + sizeof(struct iphdr)); 
                // Заголовок UDP
    unsigned char *data = buffer + sizeof(struct ether_header) + 
                sizeof(struct iphdr) + sizeof(struct udphdr); 
                // Данные полезной нагрузки
    
    struct sockaddr_ll sa; // Структура для хранения адреса уровня канала
    unsigned char src_mac[6]; // Буфер для хранения MAC-адреса отправителя

    // Получение MAC-адреса интерфейса
    if (get_mac_address(ifname, src_mac) == -1) {
        fprintf(stderr, "Failed to get MAC of interface: %s\n", ifname);
        return -1;
    }

    // Создание RAW сокета
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    // Обнуление буфера
    memset(buffer, 0, ETH_FRAME_LEN);

    // Заполнение Ethernet заголовка
    memcpy(eth_header->ether_shost, src_mac, 6); // MAC-адрес отправителя
    memcpy(eth_header->ether_dhost, dest_mac, 6); // MAC-адрес получателя
    eth_header->ether_type = htons(ETH_P_IP); // Тип протокола (IP)

    // Заполнение IP заголовка
    ip_header->ihl = 5; // Длина заголовка (в 32-битных словах)
    ip_header->version = 4; // Версия протокола (IPv4)
    ip_header->tos = 0; // Тип сервиса (0)
    ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + 
                payload_len); // Общая длина пакета
    ip_header->id = htonl(54321); // Идентификатор пакета
    ip_header->frag_off = 0; // Смещение фрагмента (0)
    ip_header->ttl = 255; // Время жизни (TTL)
    ip_header->protocol = IPPROTO_UDP; // Протокол (UDP)
    ip_header->check = 0; // Контрольная сумма (пока 0)
    inet_pton(AF_INET, src_ip, &ip_header->saddr); // IP-адрес отправителя
    inet_pton(AF_INET, dest_ip, &ip_header->daddr); // IP-адрес получателя
    ip_header->check = checksum(ip_header); 
                // Вычисление контрольной суммы IP-заголовка

    // Заполнение UDP заголовка
    udp_header->source = htons(src_port); // Порт отправителя
    udp_header->dest = htons(dest_port); // Порт получателя
    udp_header->len = htons(sizeof(struct udphdr) + payload_len); 
                // Длина UDP пакета
    udp_header->check = 0; // Контрольная сумма (0, не используется)

    // Копирование полезной нагрузки в буфер
    memcpy(data, payload, payload_len);

    // Заполнение структуры sockaddr_ll
    memset(&sa, 0, sizeof(sa));
    sa.sll_family = AF_PACKET; // Семейство адресов (AF_PACKET)
    sa.sll_ifindex = if_nametoindex(ifname); // Индекс интерфейса
    sa.sll_halen = 6; // Длина MAC-адреса
    memcpy(sa.sll_addr, dest_mac, 6); // MAC-адрес получателя

    // Отправка пакета
    if (
        sendto(sockfd, buffer, sizeof(struct ether_header) + 
                    sizeof(struct iphdr) + sizeof(struct udphdr) + 
                    payload_len, 0, (struct sockaddr *)&sa, sizeof(sa)) 
        < 0) {
            
        perror("sendto");
        close(sockfd);
        return -1;
    }

    close(sockfd); // Закрытие сокета
    return 0;
}

/**
 * @brief Прием RAW пакета
 * 
 * @param ifname Имя интерфейса
 * @param buffer Буфер для хранения принятого пакета
 * @param buf_len Длина буфера
 * @param expected_dest_port Ожидаемый порт назначения
 * @return int Длина принятого пакета при успехе, -1 при ошибке
 */
int receive_raw_packet(const char *ifname, char *buffer, int buf_len,
            unsigned short expected_dest_port) {
    int sockfd;
    struct sockaddr_ll sa;
    socklen_t sa_len = sizeof(sa);
    struct ether_header *eth_header;
    (void)eth_header; // Отключение предупреждений
    struct iphdr *ip_header;
    //(void)ip_header; // Отключение предупреждений
    struct udphdr *udp_header;

    // Создание RAW сокета
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    if (if_nametoindex(ifname) == 0) {
        perror("if_nametoindex");
        close(sockfd);
        return -1;
    }

    // Бесконечный цикл для приема пакетов
    while (1) {
        int recv_len = recvfrom(sockfd, buffer, buf_len, 0, 
                    (struct sockaddr *)&sa, &sa_len);
        if (recv_len < 0) {
            perror("recvfrom");
            close(sockfd);
            return -1;
        }

        eth_header = (struct ether_header *)buffer;
        ip_header = (struct iphdr *)(buffer + sizeof(struct ether_header));
        udp_header = (struct udphdr *)(buffer + sizeof(struct ether_header) +
                    sizeof(struct iphdr));

        char src_ip[INET_ADDRSTRLEN];
        char dest_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(ip_header->saddr), src_ip, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(ip_header->daddr), dest_ip, INET_ADDRSTRLEN);
        printf("Received packet from %s:%d to %s:%d protocol %d\n", src_ip, ntohs(udp_header->source), dest_ip, ntohs(udp_header->dest), ip_header->protocol);


        // Проверка на UDP протокол
        if (ip_header->protocol != IPPROTO_UDP) {
            continue;
        }

        // Проверка порта назначения
        if (ntohs(udp_header->dest) == expected_dest_port) {

            printf("Received packet from server!\n");

            close(sockfd);
            return recv_len;
        }
    }
}
