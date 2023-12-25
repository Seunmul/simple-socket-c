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
char g_recv_buffer[1024];
int g_bytes_received;
time_t g_current_time;

/* MAIN */
int main(int argc, char *argv[])
{
    int client_sockfd;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    fprintf(stdout, "[CLIENT] Chat Client Program Exectued.\n");

    if (argc != 2)
    {
        fprintf(stdout, "[CLIENT] Usage: %s <Chatter Name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // 소켓 생성
    client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd == -1)
    {
        perror("[CLIENT] Error creating socket");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    server_address.sin_family = AF_INET;                       // IPv4
    server_address.sin_port = htons(SERVER_PORT);              // host to network short
    inet_pton(AF_INET, SERVER_IP, &(server_address.sin_addr)); // convert IPv4 and IPv6 addresses from text to binary form

    // 서버에 연결
    if (connect(client_sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("[CLIENT] Error occured during connecting to server");
        exit(EXIT_FAILURE);
    }

    // 연결 정보 출력
    fprintf(stdout, "[CLIENT] Connected to server \n\n\
            - Server IP Address : %s \n\
            - Server Port : %d\n\n",
            SERVER_IP,
            SERVER_PORT);
    getsockname(client_sockfd, (struct sockaddr *)&client_address, &client_address_len);
    fprintf(stdout, "[CLIENT] Client address -\n\n\
            - Client IP Address %s\n\
            - Client Port : %d\n\n",
            inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
    g_bytes_received = recv(client_sockfd, g_recv_buffer, sizeof(g_recv_buffer), 0);
    send(client_sockfd, argv[1], strlen(argv[1]), 0);
    g_recv_buffer[g_bytes_received] = '\0';
    fprintf(stdout, "[CLIENT] Received: %s\n", g_recv_buffer);
    fprintf(stdout, "[CLIENT] Logined to %s. Chatroom is ready. You can chat now!\n", argv[1]);

    // 쓰레드 생성
    pthread_t receiver_tid, sender_tid;
    pthread_create(&receiver_tid, NULL, th_receiver, (void *)&client_sockfd);
    pthread_create(&sender_tid, NULL, th_sender, (void *)&client_sockfd);

    // 쓰레드 종료 대기
    int temp = pthread_join(sender_tid, NULL);
    fprintf(stdout, "[CLIENT] Receiver thread joined with status %d\n", temp);
    pthread_cancel(receiver_tid);
    pthread_join(receiver_tid, NULL);
    close(client_sockfd);

    return 0;
}

void *th_sender(void *arg)
{
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

        send(client_sockfd, user_input, sizeof(user_input), 0);

        if (strcmp(user_input, "exit") == 0)
        {
            fprintf(stdout, "[CLIENT] Exiting ...\n");
            sleep(1);
            break;
        }
    }

    pthread_exit(NULL);
}

void *th_receiver(void *arg)
{
    int client_sockfd = *(int *)arg;
    char recv_buffer[1024];
    size_t bytes_received;

    while (1)
    {
        bytes_received = recv(client_sockfd, recv_buffer, sizeof(recv_buffer), 0);
        time(&g_current_time);
        if (bytes_received <= 0)
        {
            fprintf(stdout, "[CLIENT] Error occued during receiving data\n");
            break;
        }

        recv_buffer[bytes_received] = '\0';
        fprintf(stdout, "[CLIENT]\n\
            [TIME] %s\
            [Received Data]\n\
            %s\n",
                ctime(&g_current_time), recv_buffer);
    }

    pthread_exit(NULL);
}