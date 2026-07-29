#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 256
#define MAX_CLNT 256

struct Data
{
    char word[BUF_SIZE];
    char search_count[BUF_SIZE];
};

void *handle_clnt(void *arg);
void send_msg(char *msg, int len);
void error_handling(char *msg);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;
char filename[20] = "";

FILE *fp;

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;

    if (argc != 3)
    {
        printf("Usage : %s <port> <filename>\n", argv[0]);
        exit(1);
    }

    sprintf(filename, "%s", argv[2]);

    pthread_mutex_init(&mutx, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock);
    return 0;
}

void *handle_clnt(void *arg)
{
    int clnt_sock = *((int *)arg);
    int str_len = 0, i;
    char msg[BUF_SIZE]; // 받은 메세지

    while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
    {
        printf("Receive: %s %d\n", msg, strlen(msg));
        send_msg(msg, str_len);
        memset(msg, 0, sizeof(msg));
    }

    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++) // remove disconnected client
    {
        if (clnt_sock == clnt_socks[i])
        {
            while (i++ < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}

void send_msg(char *msg, int len) // send to all
{

    char line[100];            // txt에서 한줄씩 읽는거
    char search_data[100];     // strstr을 위한 변수
    char *result;              // strstr result 담는 애
    char final_send[BUF_SIZE]; // 최종 보낼애
    int i;
    int idx = 0;
    struct Data data[10];
    char tmp_cnt[32];
    char tmp_str[64];

    memset(final_send, 0, sizeof(final_send));
    memset(line, 0, sizeof(line));
    memset(search_data, 0, sizeof(search_data));
    memset(data, 0, sizeof(data));

    fp = fopen(filename, "rb");

    if (strlen(msg) == 0)
    {
        strcpy(final_send, " ");
    }
    else
    {

        while (fgets(line, sizeof(line), fp) != NULL)
        {
            char *search_word = strtok(line, ",");
            // printf("search in txt file: %s\n", search_word);
            strcpy(search_data, search_word);

            result = strstr(search_data, msg);
            if (result != NULL)
            {
                strcpy(data[idx].word, search_data);
                search_word = strtok(NULL, "\n");
                // printf("count : %s\n", search_word);
                strcpy(data[idx].search_count, search_word);

                // printf("count d : %s\n", data[idx].search_count);

                for (int j = 0; j < (sizeof(data) / sizeof(data[1])) - 1; j++)
                {
                    for (int z = 0; z < (sizeof(data) / sizeof(data[1])) - 1; z++)
                    {
                        if (atoi(data[z].search_count) < atoi(data[z + 1].search_count))
                        {
                            strcpy(tmp_cnt, data[z].search_count);
                            strcpy(data[z].search_count, data[z + 1].search_count);
                            strcpy(data[z + 1].search_count, tmp_cnt);
                            strcpy(tmp_str, data[z].word);
                            strcpy(data[z].word, data[z + 1].word);
                            strcpy(data[z + 1].word, tmp_str);
                        }
                    }
                }
                memset(tmp_cnt, 0, sizeof(tmp_cnt));
                memset(tmp_str, 0, sizeof(tmp_str));
                idx++;
            }
            else
            {
                // printf("cannot find %s in txt file\n", search_data);
            }
            memset(search_data, 0, sizeof(search_data));
        }

        fclose(fp);
    }
    memset(final_send, 0, sizeof(final_send));

    for (int k = 0; k < sizeof(data) / sizeof(data[1]); k++)
    {
        strcat(final_send, data[k].word);
        // printf("%s", data[k].word);
        strcat(final_send, "\n");
    }

    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++)
    {
        // write(clnt_socks[i], msg, sizeof(msg));
        // printf("Send: %s\n", msg);
        write(clnt_socks[i], final_send, sizeof(final_send));
        printf("Send: %s\n", final_send);
        memset(final_send, 0, sizeof(final_send));
    }
    printf("write!\n\n\n");
    pthread_mutex_unlock(&mutx);
}
void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}