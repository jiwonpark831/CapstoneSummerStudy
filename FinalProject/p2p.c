#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>

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

void sender(char *argv[]);
void *sender_send(void *arg);
void receiver(char *argv[]);
void *receiver_send(void *arg);
void *receiver_receive(void *arg);
void error_handling(char *msg);
void opt_setting(char *argv[]);

int max_receiver;        // -n: 최대 receiving peer수
char file_name[20] = ""; // -f: 보낼 파일 이름
int seg_size;            // -g: segment 크기, 단위는 KB

int file_size; // file 총 사이즈
int seg_count; // segment 개수
int receiver_idx = 0;
int receiver_sds[10];
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

void sender(char *argv[])
{
    int sender_sd, receiver_sd;
    struct sockaddr_in sender_adr, receiver_adr;
    int receiver_adr_sz;
    struct stat file_stat;
    pthread_t tid;
    char tmp_port[8];

    // file size 구하기
    if (stat(file_name, &file_stat) == 0)
    {
        file_size = (int)file_stat.st_size;
        printf("File size: %d\n", file_size);
    }

    // segment 개수 구하기
    if (file_size % seg_size == 0)
        seg_count = file_size / seg_size;
    else
        seg_count = (file_size / seg_size) + 1;

    printf("Segment count: %d\n", seg_count);

    sender_sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&sender_sd, 0, sizeof(sender_sd));
    sender_adr.sin_family = AF_INET;
    sender_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    sender_adr.sin_port = htons(atoi(argv[2]));

    // accept
    if (bind(sender_sd, (struct sockaddr *)&sender_adr, sizeof(sender_adr)) == -1)
        error_handling("bind() error");
    if (listen(sender_sd, 5) == -1)
        error_handling("listen() error");

    while (1)
    {
        if (max_receiver < receiver_idx)
            break;

        receiver_adr_sz = sizeof(receiver_adr);
        receiver_sd = accept(sender_sd, (struct sockaddr *)&receiver_adr, &receiver_adr_sz);

        pthread_mutex_lock(&mutex);
        receiver_sds[receiver_idx++] = receiver_sd;
        pthread_mutex_unlock(&mutex);

        // receivers ip, port 저장
        receiver_info.ip[receiver_idx] = ntohl(receiver_adr.sin_addr.s_addr);
        read(receiver_sd, tmp_port, sizeof(tmp_port));
        receiver_info.port[receiver_idx] = atoi(tmp_port);

        pthread_create(&tid, NULL, sender_send, (void *)&receiver_sd);
        pthread_detach(tid);
    }
    close(sender_sd);
}

void *sender_send(void *arg)
{
    int receiver_sock = *((int *)arg);
    for (int i = 0; i < seg_count; i++)
    {
        if (i % max_receiver == receiver_idx)
        {
            // segment each size 보내기
            // 누가 뭐보낼건지

            if (i + 1 != seg_count)
                write(receiver_sds[i], &seg_size, sizeof(seg_size));
            else
            {
                // write(receiver_sds[i], 남은 seg_size, sizeof(seg_size));
            }

            struct Pkt pkt[seg_size / 1000];
            for (int j = 0; j < seg_size / 1000; j++)
            {
                write(receiver_sds[i], &receiver_info, sizeof(receiver_info));
                // write data 1000씩 보내기
                write(receiver_sds[i], &pkt[j].size, sizeof(pkt[j].size));
                write(receiver_sds[i], pkt[j].data, sizeof(pkt[j].data));
                // time 측정을 위한 read
                read(receiver_sds[i], &pkt[j].time, sizeof(pkt[j].time));

                // 진행도 출력
                // printf("진행도 출력");
            }
        }
    }
}

void receiver(char *argv[])
{
    // sender랑 연결
    int receive_sd;
    struct sockaddr_in sender_adr;

    // p2p listen으로 연결
    int receive_p2p_accept_sd, receive_p2p_request_accpet_sd;
    struct sockaddr_in receive_p2p_accept_sd_adr, receive_p2p_request_accpet_sd_adr;
    int receive_p2p_request_accpet_sd_adr_sz;

    // p2p connect로 연결
    int receive_p2p_connect_sd;
    struct sockaddr_in receive_p2p_sender_adr;

    int write_flag = 0;
    struct Receiver_info other_receiver_info;
    int my_index;
    pthread_t send_thread, receive_thread;

    // p2p connect를 위한 listen
    receive_p2p_accept_sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&receive_p2p_accept_sd_adr, 0, sizeof(receive_p2p_accept_sd_adr));
    receive_p2p_accept_sd_adr.sin_family = AF_INET;
    receive_p2p_accept_sd_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    receive_p2p_accept_sd_adr.sin_port = htons(atoi(argv[2]));

    if (bind(receive_p2p_accept_sd, (struct sockaddr *)&receive_p2p_accept_sd_adr, sizeof(receive_p2p_accept_sd_adr)) == -1)
        error_handling("bind() error");
    if (listen(receive_p2p_accept_sd, 10) == -1)
        error_handling("listen() error");

    // sender랑 connect
    receive_sd = socket(PF_INET, SOCK_STREAM, 0);
    if (receive_sd == -1)
        error_handling("socket() error");

    memset(&sender_adr, 0, sizeof(sender_adr));
    sender_adr.sin_family = AF_INET;
    sender_adr.sin_addr.s_addr = inet_addr(argv[5]);
    sender_adr.sin_port = htons(atoi(argv[6]));

    if (connect(receive_sd, (struct sockaddr *)&sender_adr, sizeof(sender_adr)) == -1)
        error_handling("connect() error");
    else
        puts("Connected..................");

    write(receive_sd, argv[2], sizeof(argv[2]));

    // 다른 receiver 정보, seg info 등 받아오기
    read(receive_sd, &other_receiver_info, sizeof(other_receiver_info));

    // my_index보다 작은 index 값이면 accept
    for (int i = 0; i < my_index; i++)
    {

        receive_p2p_request_accpet_sd_adr_sz = sizeof(receive_p2p_request_accpet_sd_adr);
        receive_p2p_request_accpet_sd = accept(receive_p2p_accept_sd, (struct sockaddr *)&receive_p2p_request_accpet_sd_adr, &receive_p2p_request_accpet_sd_adr_sz);

        pthread_create(&send_thread, NULL, sender_send, (void *)&receive_p2p_request_accpet_sd);
        pthread_create(&receive_thread, NULL, receiver_receive, (void *)&receive_p2p_request_accpet_sd);
    }

    // my_index보다 큰 애한테 connect
    for (int j = my_index; j < seg_count; j++)
    {
        receive_p2p_connect_sd = socket(PF_INET, SOCK_STREAM, 0);
        if (receive_p2p_connect_sd == -1)
            error_handling("socket() error");

        memset(&receive_p2p_sender_adr, 0, sizeof(receive_p2p_sender_adr));
        receive_p2p_sender_adr.sin_family = AF_INET;
        receive_p2p_sender_adr.sin_addr.s_addr = inet_addr(other_receiver_info[j].ip);
        receive_p2p_sender_adr.sin_port = htons(atoi(other_receiver_info[j].port));

        if (connect(receive_p2p_connect_sd, (struct sockaddr *)&receive_p2p_sender_adr, sizeof(receive_p2p_sender_adr)) == -1)
            error_handling("connect() error");
        else
            puts("Connected..................");

        pthread_create(&send_thread, NULL, sender_send, (void *)&receive_p2p_connect_sd);
        pthread_create(&receive_thread, NULL, receiver_receive, (void *)&receive_p2p_connect_sd);
    }

    // sender에게 받은 데이터 read
    while (1)
    {
        read(receive_sd, buf, sizeof(buf));
        strcat(짱 큰 배열, buf);
        write_flag++;
    }

    // pthread join
    // pthread_join();
}

void *receiver_send(void *arg)
{
    int flag;
    // if (flag != write_flag)
    // {
    //     write()
    //         flag = write_flag;
    // }
}

void *receiver_receive(void *arg)
{
    while (1)
    {
        // read();
    }
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