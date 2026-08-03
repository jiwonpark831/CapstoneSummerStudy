#include "p2p.h"

struct Pkt
{
    char data[1000];
    int size; // data의 size
    int time;
};

struct Receiver_info
{
    int ip[10];
    int port[10];
};

int max_receiver;        // -n: 최대 receiving peer수
char file_name[20] = ""; // -f: 보낼 파일 이름
int seg_size;            // -g: segment 크기, 단위는 KB

int file_size; // file 총 사이즈
int seg_count; // segment 개수
int receiver_idx = 0;
int receiver_sds[10];
int write_flag = 0; // 다 받아온 후 이제 write하라는 flag

pthread_mutex_t mutex;
struct Receiver_info receiver_info;

int main(int argc, char *argv[])
{
    pthread_mutex_init(&mutex, NULL);
    opt_setting(argv);

    if (argv[3] == "-s")
        sender(argv);
    else if (argv[3] == "-r")
        receiver(argv);
    else
        printf("[Usage] Missing type(sender or receiver)\n");
}

void opt_setting(char *argv[])
{
    // sender
    if (argv[3] == "-s")
    {
        if (argv[4] == "-n")
            max_receiver = atoi(argv[5]);
        else
            printf("[Usage] Missing option -n(maximum number of receiver)\n");

        if (argv[6] == "-f")
            sprintf(file_name, "%s", argv[7]);
        else
            printf("[Usage] Missing option -f(file name)\n");

        if (argv[8] == "-g")
            seg_size = atoi(argv[9]) * 1000;
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