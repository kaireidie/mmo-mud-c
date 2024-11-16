#include <stdio.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <mysql/mysql.h>

#include "Opcodes.h"
#include "Packet.h"

#define BUFFSIZE 1024
#define MAX_EVENTS 10

/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

void resp(int fd, struct RESPONSE *response) {
    printf("отправляю пакет с ответом\n");
    send(fd, response, sizeof(struct RESPONSE), 0);

}



void reg(int fd, struct PACKET *received_packet){
    printf("REG\n");
    printf("Opcode: %u\n", received_packet->opcode);
    printf("Login: %s\n", received_packet->login);

    printf("Hash: ");
    for (int i = 0; i < 64; i++)
    {
        printf("%02x", received_packet->hash[i]);
    }
    printf("\n");
    printf("\n-------------------------------------------\n");

    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    conn = mysql_init(NULL);
    mysql_real_connect(conn, "127.0.0.1", "root", "root", "db", 3306, NULL, 0);

    // Формируем запрос с использованием TO_BASE64()
    char query[512];  // Увеличиваем размер буфера для SQL-запроса
    sprintf(query, "INSERT INTO users (login, password_hash) VALUES ('%s', TO_BASE64('%s'));",
                                            received_packet->login, received_packet->hash);

    mysql_query(conn, query);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT failed: %s\n", mysql_error(conn));
        struct RESPONSE response;
        response.opcode = OP_ERROR;
        resp(fd, &response);
        mysql_close(conn);
    } else {
        printf("Query executed successfully\n");
        struct RESPONSE response;
        response.opcode = OP_USER_REGISTER_SUCCESS;
        resp(fd, &response);
        mysql_close(conn);
    }
}


/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/


void vhod(int fd, struct PACKET *received_packet){
    printf("VHOD\n");
    printf("Opcode: %u\n", received_packet->opcode);
    printf("Login: %s\n", received_packet->login);

    printf("Hash: ");
    for (int i = 0; i < 64; i++) {
        printf("%02x", received_packet->hash[i]);
    }
    printf("\n-------------------------------------------\n");

}


/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/


// Пришел пакет проверяем опкод
void CheckOpcode(int fd, char *buff){
    struct PACKET *received_packet = (struct PACKET *)buff;
    if (received_packet->opcode == OP_USER_REGISTER){
        //printPacket(received_packet);
        reg(fd, received_packet);
    }
    else if (received_packet->opcode == OP_USER_LOGIN){
        //printPacket(received_packet);
        vhod(fd, received_packet);
    }
    else{
        printf("OPCODE != 1 || 2 or error");
    }
}


/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/


int main () {

    /* SOCKET */
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in ad = {
        .sin_family = AF_INET,
        .sin_port = htons(12345),
        .sin_addr.s_addr = INADDR_ANY,
    };

    bind(s, (struct sockaddr *)&ad, sizeof(ad));
    listen(s, SOMAXCONN);


    /*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/


    /* EPOLL */
    int epoll_fd = epoll_create1(0);                                 /* Вы создаете новый экземпляр epoll, который будет использоваться для мониторинга событий на сокетах.                      */
    /*                                                                                                                          */
    struct epoll_event sock = {                                    /*                                                                                                                          */
        .events = EPOLLIN,                                        /* Вы создаете событие sock для вашего сокета s и добавляете его в epoll для мониторинга событий EPOLLIN (входящие данные). */
        .data.fd = s,                                            /* (Вы создаете событие sock для вашего сокета s чтобы отслеживать входящие данные (EPOLLIN).                               */
    };                                                          /*                                                                                                                          */
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s, &sock);              /*Сокет добавляется в epoll с помощью epoll_ctl.                                                                            */


    /* CYCLE */
    struct epoll_event events[MAX_EVENTS];                   /* Массив структур для хранения информации о событиях для их оброботки в цикле */
    while (1) {
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);             /*Бесконечный цикл, в котором epoll_wait блокируется, ожидая события от зарегистрированных сокетов.*/
        /*Когда событие происходит, оно сохраняется в массиве events.*/
        for (int i = 0; i < event_count; i++) {                                     //
            if (events[i].data.fd == s) {                                           //
                int a = accept(s, NULL, NULL);                                      //
                    //
                struct epoll_event client_event = {                                 //
                    .events = EPOLLIN,                                              //  Если событие связано с серверным сокетом (s), вызывается accept, чтобы принять новое соединение.
                    .data.fd = a,                                                   //  Новый сокет (a) добавляется в epoll для отслеживания событий.
                };                                                                  //  Если событие связано с клиентским сокетом, данные считываются в буфер buff с помощью recv, и,
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, a, &client_event);               //  если данные были получены, они выводятся на экран. После этого сокет закрывается.
            } else {                                                                //
                char buff[BUFFSIZE];                                                //
                int bytes_received = recv(events[i].data.fd, buff, BUFFSIZE, 0);    //
                if (bytes_received > 0) {                                           //
                    printf("Получено сообщение\n");
                    CheckOpcode(events[i].data.fd, buff);                                              // Если пришел пакет проверяем опкод

                }else if (bytes_received == 0) {                                    //  Если байтов пришло 0 то клиент отключился
                    printf("Клиент отключился.\n");                                 //
                    close(events[i].data.fd);                                       //
                }                                                                   //
            }
        }
    }

    close(epoll_fd);                                                                //закрытие епула
    close(s);
}
