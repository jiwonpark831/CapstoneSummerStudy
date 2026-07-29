#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <termios.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thread_return;
    struct termios old;

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    if (tcgetattr(STDIN_FILENO, &old) < 0)
        perror("tcgetattr");

    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &old) < 0)
        perror("tcsetattr");

    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);
    close(sock);

    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old) < 0)
        perror("tcsetattr");

    return 0;
}

void *send_msg(void *arg) // send thread main
{
    char search[BUF_SIZE];
    char ch;
    char ch_str[2];

    memset(search, 0, sizeof(search));

    int sock = *((int *)arg);
    while (1)
    {
        memset(ch_str, 0, sizeof(ch_str));
        ch = getchar();
        if (ch == '\b')
        {
            search[(strlen(search)) - 1] = '\0';
        }
        else
        {
            ch_str[0] = ch;
            ch_str[1] = '\0';
            strcat(search, ch_str);
        }
        write(sock, search, sizeof(search));
        printf("Search Word: %s\r", search);
    }
    return NULL;
}

void *recv_msg(void *arg) // read thread main
{
    int sock = *((int *)arg);
    char result[1024];
    int str_len;
    while (1)
    {
        memset(result, 0, sizeof(result));

        str_len = read(sock, result, sizeof(result));
        if (str_len == -1)
            return (void *)-1;
        result[str_len] = 0;
        printf("===================================\n");
        printf("%s\r", result);
    }
    return NULL;
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
