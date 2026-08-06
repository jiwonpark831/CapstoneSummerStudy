#include "p2p.h"

int max_receiver;        // -n: 최대 receiving peer수
char file_name[20] = ""; // -f: 보낼 파일 이름
int seg_size;            // -g: segment 크기, 단위는 KB

int file_size; // file 총 사이즈
int seg_count; // segment 개수
int receiver_sds[10];
int receiver_adrs[10];
short receiver_ports[10];
int write_flag = 0; // 다 받아온 후 이제 write하라는 flag

pthread_mutex_t mutex;
// struct Receiver_info receiver_info;

int main(int argc, char *argv[])
{
    pthread_mutex_init(&mutex, NULL);
    opt_setting(argv);

    if (strcmp(argv[3], "-s") == 0)
    {
        printf("[Sender] start\n");
        sender(argv);
    }
    else if (strcmp(argv[3], "-r") == 0)
    {
        printf("[Receiver] start\n");
        receiver(argv);
    }
    else
        printf("[Usage] Missing type(sender or receiver)\n");
}

void opt_setting(char *argv[])
{
    // sender
    if (strcmp(argv[3], "-s") == 0)
    {
        if (strcmp(argv[4], "-n") == 0)
        {
            max_receiver = atoi(argv[5]);
            printf("=SET OPTION= maxreceiver: %d\n", max_receiver);
        }
        else
            printf("[Usage] Missing option -n(maximum number of receiver)\n");

        if (strcmp(argv[6], "-f") == 0)
        {
            sprintf(file_name, "%s", argv[7]);
            printf("=SET OPTION= file_name: %s\n", file_name);
        }
        else
            printf("[Usage] Missing option -f(file name)\n");

        if (strcmp(argv[8], "-g") == 0)
        {
            seg_size = atoi(argv[9]) * 1000;
            printf("=SET OPTION= seg_size: %d\n", seg_size);
        }
        else
            printf("[Usage] Missing -g(segment size)\n");
    }
}
void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

int read_exact(int fd, void *buf, size_t len)
{
    size_t total = 0;
    char *ptr = (char *)buf;

    while (total < len)
    {
        ssize_t n = read(fd, ptr + total, len - total);
        if (n <= 0)
            return -1;
        total += n;
    }

    return 0;
}

int write_all(int fd, const void *buf, size_t len)
{
    size_t total = 0;
    const char *ptr = (const char *)buf;

    while (total < len)
    {
        ssize_t n = write(fd, ptr + total, len - total);
        if (n <= 0)
            return -1;
        total += n;
    }

    return 0;
}