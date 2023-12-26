/***
 * @file server.c
 * @brief simple chat server
 * @date 2023-12-25
 * @author GeonhaPark <geonhab504@gmail.com>
 */

/* HEADERS */
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* DEFINE */
#define DEBUG 0
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9999

#define MAX_CHATTER_LIM 3
#define SOCKFD_LISTEN_QUEUE_LEN MAX_CHATTER_LIM /* size of request queue */
#define QUEUE_BUFFER_SIZE 20

/* STRUCTS */
typedef struct _client_info
{
    int num;
    int sockfd;
    pthread_t tid;
    char nickname[20];
} ClientInfo;

typedef struct
{
    char data[1024]; // 데이터의 예시로 문자열을 담는다고 가정
    char *nickname;
    int client_sockfd;
} Data; // 데이터를 담을 구조체

typedef struct
{
    Data items[QUEUE_BUFFER_SIZE];
    int front;
    int rear;
    pthread_mutex_t mutex;
} Queue; // 큐 구조체 정의

/* FUNCTIONS */
static inline void show_cli_list();
static inline void init_mutex();
static inline void destroy_mutex();
static void enqueue(const Data *item);
static void dequeue(Data *item);
void *receiver_thread(void *arg);
void *server_thread(void *arg);
void *sender_thread(void *arg);

/* GLOBAL VARIABLES */
int g_cli_choice = 1;
int g_total_client_num; // scounts client connections
time_t g_current_time;
pthread_mutex_t
    g_client_num_mut,
    g_sender_mutex,
    g_cli_sync_mutex[2]; // 0 : cli choice variable, 1 : Thread Sync
pthread_cond_t
    g_sender_cond,
    g_cli_sync_cond;
Queue g_sharedQueue = {.front = -1, .rear = -1, .mutex = PTHREAD_MUTEX_INITIALIZER};
ClientInfo *g_client_info_arr[MAX_CHATTER_LIM];

/* MAIN */
int main(int argc, char *argv[])
{
    int server_sockfd;                        /* socket file descriptors */
    struct sockaddr_in server_address;        /* structure to hold server's address */
    uint16_t port;                            /* protocol port number */
    pthread_t sender_tid = 0, server_tid = 0; /* variable to hold thread ID */

    if ((port = ((argc > 1) ? atoi(argv[1]) : SERVER_PORT)) <= 0)
    {
        fprintf(stdout, "[SERVER] bad port number %s/n", argv[1]);
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "[SERVER] Chat Client Program Exectued.\n");
    init_mutex();

    /* setup socket settings */
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);           // server_sockfd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    server_address.sin_family = AF_INET;                       // set family to Internet
    server_address.sin_port = htons(port);                     // change port number memory from pc's endian
    inet_pton(AF_INET, SERVER_IP, &(server_address.sin_addr)); // set the IP address : SERVER_IP is Defined by MACRO
    // server_address.sin_addr.s_addr = htonl(INADDR_ANY); // set the local IP address : INADDR_ANY is all local interfaces

    if (server_sockfd < 0)
    {
        fprintf(stdout, "[SERVER] Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    /* bind socket */
    if (bind(server_sockfd, (const struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        fprintf(stdout, "[SERVER] Socket bind failed\n");
        exit(EXIT_FAILURE);
    }

    /* open socket */
    if (listen(server_sockfd, SOCKFD_LISTEN_QUEUE_LEN) < 0)
    {
        fprintf(stdout, "[SERVER] Socket listen failed\n");
        exit(EXIT_FAILURE);
    }

    /* shows socket sconfiguration info */
    fprintf(stdout, "[SERVER] Server up and running.\n\n\
            - Server IP Address : %s \n\
            - Server Port : %d\n\n",
            inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

    if (pthread_create(&sender_tid, NULL, sender_thread, NULL) < 0)
    {
        perror("[SERVER] ERROR Occured while load Sender Thread.");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&server_tid, NULL, server_thread, (void *)&server_sockfd) < 0)
    {
        perror("[SERVER] ERROR Occured while load Server Thread.");
        exit(EXIT_FAILURE);
    }

    show_cli_list();
    while (1)
    {
        static int cli_choice = 0;
        static int continue_flag = 0;
        scanf("%d", &cli_choice);

        if (g_cli_choice == cli_choice)
        {
            fprintf(stdout, "\n========================================\n");
            fprintf(stdout, "[SERVER_CLI] You selected same option. Please try other Option.\n");
            fprintf(stdout, "========================================\n\n");
            continue;
        }

        switch (cli_choice)
        {
        case 0:
            fprintf(stdout, "\n========================================\n");
            fprintf(stdout, "[SERVER_CLI] You selected Help Option.\n");
            fprintf(stdout, "========================================\n\n");
            show_cli_list();
            continue_flag = 1;
            break;
        case 1:
            fprintf(stdout, "\n========================================\n");
            fprintf(stdout, "[SERVER_CLI] Socket Server Opened.\n");
            fprintf(stdout, "[SERVER_CLI] Now Socket Server Will Spawn Client Threads.\n");
            fprintf(stdout, "========================================\n\n");
            break;
        case 2:
            fprintf(stdout, "\n========================================\n");
            fprintf(stdout, "[SERVER_CLI] You selected Exit Option.\n");
            fprintf(stdout, "========================================\n\n");
            break;
        default:
            fprintf(stdout, "\n========================================\n");
            fprintf(stdout, "[SERVER_CLI] You selected Invalid Option. Please try other Option\n");
            fprintf(stdout, "========================================\n\n");
            show_cli_list();
            continue_flag = 1;
            break;
        }

        if (continue_flag == 1)
        {
            continue;
        }

        pthread_mutex_lock(&g_cli_sync_mutex[0]);
        g_cli_choice = cli_choice;
        pthread_mutex_unlock(&g_cli_sync_mutex[0]);

        if (g_cli_choice == 2)
            break;
    }
    fprintf(stdout, "[SERVER] CLI cloesd.\n");
    pthread_detach(server_tid);
    pthread_cancel(sender_tid);
    pthread_join(sender_tid, NULL);
    destroy_mutex();
    close(server_sockfd);
    fprintf(stdout, "[SERVER] Server closed.\n");
    exit(EXIT_SUCCESS);
}

/* Other Functions */
static inline void init_mutex()
{
    pthread_mutex_init(&g_client_num_mut, NULL);
    pthread_mutex_init(&g_sender_mutex, NULL);
    pthread_mutex_init(&g_sharedQueue.mutex, NULL);
    for (int i = 0; i < 3; i++)
    {
        pthread_mutex_init(&g_cli_sync_mutex[i], NULL);
    }
    pthread_cond_init(&g_cli_sync_cond, NULL);
    pthread_cond_init(&g_sender_cond, NULL);
    return;
}

static inline void destroy_mutex()
{
    pthread_mutex_destroy(&g_client_num_mut);
    pthread_mutex_destroy(&g_sender_mutex);
    pthread_mutex_destroy(&g_sharedQueue.mutex);
    for (int i = 0; i < 3; i++)
    {
        pthread_mutex_destroy(&g_cli_sync_mutex[i]);
    }
    pthread_cond_destroy(&g_cli_sync_cond);
    pthread_cond_destroy(&g_sender_cond);
    return;
}

static inline void show_cli_list()
{
    fprintf(stdout, "\n========================================\n");
    fprintf(stdout, "[SERVER CLI] 0. CLI Help\n");
    fprintf(stdout, "[SERVER CLI] 1. Open New Clients Threads(Default)\n");
    fprintf(stdout, "[SERVER CLI] 2. Exit\n");
    fprintf(stdout, "========================================\n\n");
    fprintf(stdout, "[SERVER CLI] Enter your cli_choice: \n\n");
    return;
}

static void enqueue(const Data *item) // 데이터를 큐에 삽입하는 함수
{
    pthread_mutex_lock(&g_sharedQueue.mutex);

    if ((g_sharedQueue.rear + 1) % QUEUE_BUFFER_SIZE == g_sharedQueue.front)
    {
#if DEBUG
        fprintf(stderr, "[QUEUE] Queue is full. Data not enqueued.\n"); // 큐가 가득 찬 경우
#endif
    }
    else
    {
        if (g_sharedQueue.front == -1)
        {
            g_sharedQueue.front = 0;
        }
        g_sharedQueue.rear = (g_sharedQueue.rear + 1) % QUEUE_BUFFER_SIZE;
        g_sharedQueue.items[g_sharedQueue.rear] = *item;
#if DEBUG
        fprintf(stderr, "[QUEUE] Data enqueued.\n");
#endif
    }

    pthread_mutex_unlock(&g_sharedQueue.mutex);
    return;
}

static void dequeue(Data *item) // 데이터를 큐에서 추출하는 함수
{
    pthread_mutex_lock(&g_sharedQueue.mutex);

    if (g_sharedQueue.front == -1)
    {
#if DEBUG
        fprintf(stderr, "[QUEUE] Queue is empty. No data to dequeue.\n"); // 큐가 비어 있는 경우
#endif
    }
    else
    {
        *item = g_sharedQueue.items[g_sharedQueue.front];
        if (g_sharedQueue.front == g_sharedQueue.rear)
        {
            g_sharedQueue.front = g_sharedQueue.rear = -1;
        }
        else
        {
            g_sharedQueue.front = (g_sharedQueue.front + 1) % QUEUE_BUFFER_SIZE;
        }
#if DEBUG
        fprintf(stderr, "[QUEUE] Data dequeued.\n");
#endif
    }
    pthread_mutex_unlock(&g_sharedQueue.mutex);
    return;
}

void *sender_thread(void *arg)
{
    while (1)
    {
        pthread_cond_wait(&g_sender_cond, &g_sender_mutex);
        Data data;
        char send_data[1044];
        dequeue(&data);
#if DEBUG
        fprintf(stdout, "[SERVER] Sending Data : %d\n", data.client_sockfd);
        fprintf(stdout, "[SERVER] Sending Data : %s\n", data.nickname);
        fprintf(stdout, "[SERVER] Sending Data : %s\n", data.data);
#endif
        sprintf(send_data, "(USER NAME : %s) ", data.nickname);
        strcat(send_data, data.data);

        pthread_mutex_lock(&g_client_num_mut);
        for (int i = 0; i < g_total_client_num; i++)
        {
            // if (g_client_info_arr[i]->sockfd != data.client_sockfd)
            // {
            send(g_client_info_arr[i]->sockfd, send_data, strlen(send_data), 0);
            // }
        }
        pthread_mutex_unlock(&g_client_num_mut);
    }
    pthread_exit(NULL);
}

void *server_thread(void *arg)
{
    char sendbuf[1024]; /* buffer for string the server sends */
    int idx = 0;
    int mx_chat = MAX_CHATTER_LIM;
    int tmp_sockfd = 0;
    int bytes_received; /* length of message received from client */
    int cli_choice = 0;
    int chatter_overflow_flag = 0;
    int server_sockfd = *((int *)arg);
    struct sockaddr_in client_address; /* structure to hold client's address */
    socklen_t client_address_len = sizeof(client_address);
    ClientInfo client_info[MAX_CHATTER_LIM];

    while (1)
    {
        pthread_mutex_lock(&g_cli_sync_mutex[0]);
        cli_choice = g_cli_choice;
        pthread_mutex_unlock(&g_cli_sync_mutex[0]);

        switch (cli_choice)
        {
        case 1: // 클라이언트 접속을 허용하는 코드를 여기에 작성합니다.
            fprintf(stdout, "[SERVER] Listening... (New Clients can Join)\n");
            fprintf(stdout, "[SERVER] Waiting for connection ...\n");

            if ((tmp_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_address_len)) < 0)
            {
                fprintf(stdout, "[SERVER] Acception failed, %d\n", tmp_sockfd);
                continue;
            }

            pthread_mutex_lock(&g_client_num_mut);
            if (g_total_client_num < MAX_CHATTER_LIM)
            {
                idx = g_total_client_num;
                g_total_client_num++;
            }
            else
            {
                chatter_overflow_flag = 1;
            }
            pthread_mutex_unlock(&g_client_num_mut);

            if (chatter_overflow_flag == 1)
            {
                fprintf(stdout, "[SERVER] Connection is not permitted, there are already MAX Chatters : %d\n", mx_chat);
                fprintf(stdout, "[SERVER] Connection is not permitted, Server Status is now 2 - Not Spawn Revceiver Threads\n");
                close(tmp_sockfd);
                continue;
            }

            sprintf(sendbuf, "Welcome. You are \'%d\' Chatter", client_info[idx].num);
            send(tmp_sockfd, sendbuf, strlen(sendbuf), 0);
            bytes_received = recv(tmp_sockfd, client_info[idx].nickname, sizeof(client_info[idx].nickname) - 1, 0); // g_nickname_arr 19 character available

            client_info[idx].num = 0;
            client_info[idx].sockfd = tmp_sockfd;
            client_info[idx].nickname[bytes_received] = '\0';
            g_client_info_arr[idx] = (ClientInfo *)&client_info[idx];

            fprintf(stdout, "\n\n===============================\n");
            fprintf(stdout, "[SERVER] Connection is permitted, Total clients : %d\n", g_total_client_num);
            fprintf(stderr, "[SERVER] Client connected from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            fprintf(stdout, "[SERVER] USER %d Name : %s\n", client_info[idx].num, client_info[idx].nickname);

            if (pthread_create(&(client_info[idx].tid), NULL, receiver_thread, (void *)&client_info[idx]) < 0)
            {
                pthread_mutex_lock(&g_client_num_mut);
                g_total_client_num--;
                pthread_mutex_unlock(&g_client_num_mut);
                close(client_info[idx].sockfd);
                fprintf(stdout, "[SERVER] [ERROR] receiver_thread creatation Failed:%ld\n", client_info[idx].tid);
                fprintf(stdout, "[SERVER] [ERROR] Close Client Connection %d, now Total clients : %d\n", client_info[idx].sockfd, g_total_client_num);
                fprintf(stdout, "===============================\n");
                break;
            }
            fprintf(stdout, "[SERVER] Receiver Thread ID : %ld\n", client_info[idx].tid);
            fprintf(stdout, "===============================\n\n");
            break;

        case 2:
            fprintf(stdout, "[SERVER] Exiting ...\n");
            break;

        default:
            fprintf(stdout, "[SERVER] Invalid OPTION.\n");
            break;
        }
        if (cli_choice == 2)
            break;
    }
    for (int i = 0; i < g_total_client_num; i++)
    {
        close(client_info[i].sockfd);
    }
    sleep(1);
    for (int i = 0; i < g_total_client_num; i++)
    {
        pthread_join(client_info[i].tid, NULL);
    }
    pthread_exit(NULL);
}

void *receiver_thread(void *arg)
{
    char recvbuf[1024];
    int bytes_received; /* length of message received from client */

    ClientInfo client_info = {
        .tid = ((ClientInfo *)arg)->tid,
        .num = ((ClientInfo *)arg)->num,
        .sockfd = ((ClientInfo *)arg)->sockfd,
        .nickname = ""};
    strcpy(client_info.nickname, ((ClientInfo *)arg)->nickname);

    /* save to Share Queue */
    enqueue(&(Data){.client_sockfd = client_info.sockfd, .nickname = client_info.nickname, .data = "is joined to chat."});
    pthread_mutex_lock(&g_sender_mutex);
    pthread_cond_signal(&g_sender_cond);
    pthread_mutex_unlock(&g_sender_mutex);

#if DEBUG
    fprintf(stdout, "DEBUG -- [SERVER] thread id:%ld\n", pthread_self());
#endif

    while (1)
    {
        Data recv_data = {
            .client_sockfd = client_info.sockfd,
            .nickname = client_info.nickname,
            .data = ""};
        bytes_received = recv(client_info.sockfd, recvbuf, sizeof(recvbuf), 0);
        if (bytes_received < 0)
        {
            fprintf(stdout, "[SERVER-RECEIVER] [ERROR] Error occued during receiving data\n");
            continue;
        }
        if (bytes_received == 0)
        {
            fprintf(stdout, "[SERVER-RECEIVER] [ERROR] Socket closed\n");
            break;
        }
        recvbuf[bytes_received] = '\0';
        time(&g_current_time);
        fprintf(stdout, "[SERVER-RECEIVER]\n\
            [Time] %s\
            [From] %s\n\
            [Received Data]\n\
            %s\n",
                ctime(&g_current_time), client_info.nickname, recvbuf);

        if (strcmp(recvbuf, "exit") == 0) // exit -> send "has left chat."
        {
            strcpy(recv_data.data, "has left chat.");
            enqueue(&recv_data);
            pthread_mutex_lock(&g_sender_mutex);
            pthread_cond_signal(&g_sender_cond);
            pthread_mutex_unlock(&g_sender_mutex);
            break;
        }
        else
        {
            strcpy(recv_data.data, recvbuf); // else -> send received data
            enqueue(&recv_data);
            pthread_mutex_lock(&g_sender_mutex);
            pthread_cond_signal(&g_sender_cond);
            pthread_mutex_unlock(&g_sender_mutex);
        }
    }

    pthread_mutex_lock(&g_client_num_mut);
    g_total_client_num--;
    pthread_mutex_unlock(&g_client_num_mut);

    close(client_info.sockfd);
    fprintf(stdout, "[SERVER] Client %d is disconnected.\n", client_info.num);
    fprintf(stdout, "[SERVER] Total clients : %d\n", g_total_client_num);

    pthread_exit(NULL);
}