#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <mysql/mysql.h>

#include "Opcodes.h"
#include "Packet.h"
#include "engine.h"

#define BUFFSIZE 1024
#define MAX_EVENTS 10

void acceptWorld(int fd, WorldPacket *worldPacket){
    printf("Отправляю пакет с апрувом на клиент\n");
    send(fd, worldPacket, sizeof(WorldPacket), 0); //for debug client
    sleep(2); //for debug client
    send(fd, worldPacket, sizeof(WorldPacket), 0);

}

void resp(int fd, Response *response) {
    printf("Отправляю пакет с ответом\n");
    send(fd, response, sizeof(Response), 0);
}

void error_client(int fd) {
    Response response;
    printf("Отправляю пакет с ошибкой\n");
    response.opcode = OP_ERROR_CLIENT;
    resp(fd, &response);
}

void reg(int fd, Packet *received_packet) {
    printf("REG\n");
    printf("Opcode: %u\n", received_packet->opcode);
    printf("Login: %s\n", received_packet->login);

    printf("Hash: ");
    for (int i = 0; i < 64; i++) {
        printf("%02x", received_packet->hash[i]);
    }
    printf("\n");
    printf("\n-------------------------------------------\n");

    MYSQL *conn = mysql_init(NULL);
    mysql_real_connect(conn, "127.0.0.1", "root", "root", "db", 3306, NULL, 0);

    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO users (login, password_hash) VALUES ('%s', '%s');",
             received_packet->login, received_packet->hash);

    printf("SQL Query: %s\n", query);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT failed: %s\n", mysql_error(conn));
        Response response;
        response.opcode = OP_ERROR;
        resp(fd, &response);
        mysql_close(conn);
    } else {
        printf("Query executed successfully\n");
        Response response;
        response.opcode = OP_USER_REGISTER_SUCCESS;
        resp(fd, &response);
        mysql_close(conn);
    }
}

void vhod(int fd, Packet *received_packet) {
    printf("VHOD\n");
    printf("Opcode: %u\n", received_packet->opcode);
    printf("Login: %s\n", received_packet->login);

    printf("Hash: ");
    for (int i = 0; i < 64; i++) {
        printf("%02x", received_packet->hash[i]);
    }
    printf("\n-------------------------------------------\n");

    MYSQL *conn = mysql_init(NULL);
    mysql_real_connect(conn, "127.0.0.1", "root", "root", "db", 3306, NULL, 0);

    char query[512];
    snprintf(query, sizeof(query),
             "SELECT EXISTS(SELECT 1 FROM users WHERE login='%s' AND password_hash=TRIM('%s'))",
             received_packet->login, received_packet->hash);

    printf("SQL Query: %s\n", query);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT failed: %s\n", mysql_error(conn));
        Response response;
        response.opcode = OP_ERROR;
        resp(fd, &response);
        mysql_close(conn);
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);

        if (row) {
            int exists = atoi(row[0]);
            if (exists) {
                printf("Пользователь существует.\n");
                WorldPacket worldPacket;
                worldPacket.opcode = OP_USER_LOGIN_SUCCESS;

                strncpy(worldPacket.ip_addres, "127.0.0.1", sizeof(worldPacket.ip_addres) - 1);
                worldPacket.ip_addres[sizeof(worldPacket.ip_addres) - 1] = '\0';

                worldPacket.port = 12344;
                worldPacket.severid = 1101;

                acceptWorld(fd, &worldPacket);
            } else {
                printf("Пользователь не существует.\n");
                Response response;
                response.opcode = OP_USER_LOGIN_FAILED;
                resp(fd, &response);
            }
        }

        mysql_free_result(res);
    }

    mysql_close(conn);
}

void CheckOpcode(int fd, char *buff) {
    printf("FD = %d\n", fd);
    Packet *received_packet = (Packet *)buff;

    if (received_packet->opcode == OP_USER_REGISTER) {
        reg(fd, received_packet);
    } else if (received_packet->opcode == OP_USER_LOGIN) {
        vhod(fd, received_packet);
    } else {
        error_client(fd);
        printf("OPCODE ERROR\nОтключаю клиент %d\n", fd);
        close(fd);
    }
}

int main() {
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in ad = {
        .sin_family = AF_INET,
        .sin_port = htons(12345),
        .sin_addr.s_addr = INADDR_ANY,
    };

    bind(s, (struct sockaddr *)&ad, sizeof(ad));
    listen(s, SOMAXCONN);

    int epoll_fd = epoll_create1(0);
    struct epoll_event sock = {
        .events = EPOLLIN,
        .data.fd = s,
    };
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s, &sock);

    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < event_count; i++) {
            if (events[i].data.fd == s) {
                int a = accept(s, NULL, NULL);
                struct epoll_event client_event = {
                    .events = EPOLLIN,
                    .data.fd = a,
                };
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, a, &client_event);
            } else {
                char buff[BUFFSIZE];
                int bytes_received = recv(events[i].data.fd, buff, BUFFSIZE, 0);
                if (bytes_received > 0) {
                    printf("Получено сообщение\n");
                    CheckOpcode(events[i].data.fd, buff);
                } else if (bytes_received == 0) {
                    printf("Клиент id %d отключился.\n", events[i].data.fd);
                    close(events[i].data.fd);
                }
            }
        }
    }

    close(epoll_fd);
    close(s);
}
