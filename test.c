#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void *cli_thread(void *arg);

pthread_mutex_t g_cli_sync_mutex[2]; // 0 : cli choice variable, 1 : Thread Sync
pthread_cond_t g_cli_sync_cond;
pthread_t cli_tid;
int g_cli_choice = 1;

/* ========================================
PRIVATE FUNCTIONS
======================================== */
static inline void init_mutex()
{
    for (int i = 0; i < 3; i++)
    {
        pthread_mutex_init(&g_cli_sync_mutex[i], NULL);
    }
    pthread_cond_init(&g_cli_sync_cond, NULL);
}

static inline void destroy_mutex()
{
    for (int i = 0; i < 3; i++)
    {
        pthread_mutex_destroy(&g_cli_sync_mutex[i]);
    }
    pthread_cond_destroy(&g_cli_sync_cond);
}

static inline void show_cli_list()
{
    fprintf(stdout, "\n========================================\n");
    fprintf(stdout, "[SERVER CLI] 0. CLI Help\n");
    fprintf(stdout, "[SERVER CLI] 1. Open New Clients Threads(Default)ß\n");
    fprintf(stdout, "[SERVER CLI] 2. Close New Clients Threads\n");
    fprintf(stdout, "[SERVER CLI] 3. Option 3\n");
    fprintf(stdout, "[SERVER CLI] 4. Exit\n");
    fprintf(stdout, "========================================\n\n");
    fprintf(stdout, "[SERVER CLI] Enter your cli_choice: \n\n");

}

/* ========================================
MAIN INLINE FUNCTIONS
======================================== */
int main()
{
    int cli_choice = 0;
    init_mutex();

    if (pthread_create(&cli_tid, NULL, cli_thread, NULL) < 0) // 여기서 tid값은 thread내에서 pthread_self()반환값
    {
        perror("[SERVER] recv_thread create error:");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        pthread_mutex_lock(&g_cli_sync_mutex[0]);
        cli_choice = g_cli_choice;
        pthread_mutex_unlock(&g_cli_sync_mutex[0]);

        switch (cli_choice)
        {
        case 1: // 클라이언트 접속을 허용하는 코드를 여기에 작성합니다.
            fprintf(stdout, "[SERVER] Listening... (New Clients can Join)\n");
            break;
        case 2: // 클라이언트가 더이상 접속하지 못하게 하는 코드를 여기에 작성합니다.
            fprintf(stdout, "[SERVER] New Clients cannot join\n");
            // pthread_mutex_lock(&g_cli_sync_mutex[1]);
            pthread_cond_wait(&g_cli_sync_cond, &g_cli_sync_mutex[1]);
            break;
        case 3: // Option 3에 대한 코드를 여기에 작성합니다.
            fprintf(stdout, "[SERVER] Option 3.\n");
            break;
        case 4:
            fprintf(stdout, "[SERVER] Exiting ...\n");
            break;
        default:
            fprintf(stdout, "[SERVER] Invalid OPTION.\n");
            break;
        }
        sleep(1);
        if (cli_choice == 4)
            break;
    }
    pthread_join(cli_tid, NULL);
    destroy_mutex();
    return EXIT_SUCCESS;
}

void *cli_thread(void *arg)
{
    int cli_choice = 0;
    show_cli_list();
    while (1)
    {
        int continue_flag = 0;
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
            fprintf(stdout, "[SERVER_CLI] Option 1 : Socket Server Opened.\n");
            fprintf(stdout, "[SERVER_CLI] Now Socket Server Will Spawn Client Threads.\n");
            fprintf(stdout, "========================================\n\n");
            break;
        case 2:
            fprintf(stdout, "\n========================================\n");
            fprintf(stdout, "[SERVER_CLI] Option 2 : Socket Server Closed.\n");
            fprintf(stdout, "[SERVER_CLI] Now Socket Server will not spawn another Client.\n");
            fprintf(stdout, "========================================\n\n");
            break;
        case 3:
            fprintf(stdout, "\n========================================\n");
            fprintf(stdout, "[SERVER_CLI] Option 3.\n");
            fprintf(stdout, "========================================\n\n");
            break;
        case 4:
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

        if (g_cli_choice == 1 || g_cli_choice == 4)
        {
            pthread_mutex_lock(&g_cli_sync_mutex[1]);
            pthread_cond_signal(&g_cli_sync_cond);
            pthread_mutex_unlock(&g_cli_sync_mutex[1]);
        }

        if (g_cli_choice == 4)
            break;
    }
    fprintf(stdout, "[SERVER] Exiting CLI thread.\n");
    pthread_exit(NULL);
}