/***
 * @file server.c
 * @brief simple chat client
 * @date 2023-12-25
 * @author GeonhaPark <geonhab504@gmail.com>
 */

/* HEADERS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

/* DEFINE */
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9999

/* FUNCTIONS */
void *th_receiver(void *arg);
void *th_sender(void *arg);

/* GLOBAL VARIABLES */
int g_socket_stat = 0;
time_t g_current_time;
pthread_mutex_t g_sync_mut;

/* MAIN */
int main(int argc, char *argv[])
{
    int client_sockfd;
    char tmp_recv_buf[1024];
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    size_t bytes_received;
    uint16_t port;

    pthread_mutex_init(&g_sync_mut, NULL);
    if (argc < 2)
    {
        fprintf(stdout, "[CLIENT] Usage: %s <Chatter Name> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if ((port = ((argc > 2) ? atoi(argv[2]) : SERVER_PORT)) <= 0)
    {
        fprintf(stdout, "[SERVER] bad port number %s/n", argv[1]);
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "[CLIENT] Chat Client Program Exectued.\n");

    /* setup socket settings */
    client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;                       // IPv4
    server_address.sin_port = htons(port);                     // host to network short
    inet_pton(AF_INET, SERVER_IP, &(server_address.sin_addr)); // convert IPv4 and IPv6 addresses from text to binary form

    if (client_sockfd < 0)
    {
        fprintf(stdout, "[CLIENT] Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    /* server connection */
    if (connect(client_sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("[CLIENT] Error occured during connecting to server");
        exit(EXIT_FAILURE);
    }

    /* shows socket configuration info */
    getsockname(client_sockfd, (struct sockaddr *)&client_address, &client_address_len); // get client socket info
    fprintf(stdout, "[CLIENT] Connected to server \n\n\
            - Server IP Address : %s \n\
            - Server Port : %d\n\n",
            inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));
    fprintf(stdout, "[CLIENT] Client address -\n\n\
            - Client IP Address %s\n\
            - Client Port : %d\n\n",
            inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

    bytes_received = recv(client_sockfd, tmp_recv_buf, sizeof(tmp_recv_buf), 0);
    if (bytes_received <= 0) // 서버가 종료되었거나, 접속자 수가 많아 접속이 불가능한 경우
    {
        fprintf(stdout, "[CLIENT] Chat Server is not available\n");
        close(client_sockfd);
        exit(EXIT_FAILURE);
    }

    send(client_sockfd, argv[1], strlen(argv[1]), 0);
    tmp_recv_buf[bytes_received] = '\0';
    fprintf(stdout, "[CLIENT] Received: %s\n", tmp_recv_buf);
    fprintf(stdout, "[CLIENT] Logined to %s. Chatroom is ready. You can chat now!\n", argv[1]);

    // 쓰레드 생성
    pthread_t receiver_tid, sender_tid;
    pthread_create(&receiver_tid, NULL, th_receiver, (void *)&client_sockfd);
    pthread_create(&sender_tid, NULL, th_sender, (void *)&client_sockfd);

    // 쓰레드 종료 대기
    pthread_join(receiver_tid, NULL);
    sleep(1);
    pthread_cancel(sender_tid);
    pthread_join(sender_tid, NULL);

    pthread_mutex_destroy(&g_sync_mut);
    close(client_sockfd);
    return 0;
}

void *th_sender(void *arg)
{
    int status = 0;
    int client_sockfd = *(int *)arg;
    char user_input[1024];
    size_t user_input_len = 0;
    fprintf(stdout, "[CLIENT] Enter message to send (type 'exit' to quit): \n");
    while (1)
    {
        if (fgets(user_input, sizeof(user_input), stdin) != NULL)
        {
            user_input_len = strlen(user_input); // 문자열의 길이를 확인

            if (user_input_len > 0 && user_input[user_input_len - 1] == '\n') // 개행 문자('\n')을 제거
            {
                user_input[user_input_len - 1] = '\0';
                user_input_len--;
            }
        }
        else
        {
            fprintf(stdout, "[CLIENT] Error reading input.\n");
            continue;
        }

        pthread_mutex_lock(&g_sync_mut);
        status = g_socket_stat;
        pthread_mutex_unlock(&g_sync_mut);
        if (status == -1)
            break;

        send(client_sockfd, user_input, sizeof(user_input), 0);

        if (strcmp(user_input, "exit") == 0)
        {
            fprintf(stdout, "[CLIENT] Exiting ...\n");

            /* socket status */
            status = -1;
            pthread_mutex_lock(&g_sync_mut);
            g_socket_stat = status;
            pthread_mutex_unlock(&g_sync_mut);
            break;
        }
    }

    pthread_exit((void *)&status);
}

void *th_receiver(void *arg)
{
    int status = 0;
    int client_sockfd = *(int *)arg;
    char recv_buffer[1024];
    size_t bytes_received;

    while (1)
    {
        bytes_received = recv(client_sockfd, recv_buffer, sizeof(recv_buffer), 0);
        if (bytes_received < 0)
        {
            fprintf(stdout, "[CLIENT] Error occued during receiving data\n");
            status = -1;
        }
        else if (bytes_received == 0)
        {
            fprintf(stdout, "[CLIENT] Socket closed. ...\n");
            status = -1;
        }

        pthread_mutex_lock(&g_sync_mut);
        g_socket_stat = status;
        pthread_mutex_unlock(&g_sync_mut);
        if (status == -1)
            break;

        recv_buffer[bytes_received] = '\0';
        time(&g_current_time);
        fprintf(stdout, "[CLIENT]\n\
            [TIME] %s\
            [Received Data]\n\
            %s\n",
                ctime(&g_current_time), recv_buffer);
    }

    pthread_exit((void *)&status);
}