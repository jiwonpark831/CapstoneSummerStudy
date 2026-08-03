#ifndef P2P_H
#define P2P_H

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
};

struct Receiver_info
{
    int ip[10];
    short port[10];
};

void sender(char *argv[]);
void *sender_send(void *arg);
void receiver(char *argv[]);
void *peer_send(void *arg);
void *peer_receive(void *arg);
void error_handling(char *msg);
void opt_setting(char *argv[]);
void print_sender_result();
void print_receiver_result();

extern int max_receiver;        // -n: 최대 receiving peer수
extern char file_name[20] = ""; // -f: 보낼 파일 이름
extern int seg_size;            // -g: segment 크기, 단위는 KB

extern int file_size; // file 총 사이즈
extern int seg_count; // segment 개수
extern int receiver_idx = 0;
extern int receiver_sds[10];

extern int write_flag = 0; // 다 받아온 후 이제 write하라는 flag

extern pthread_mutex_t mutex;
extern struct Receiver_info receiver_info;

#endif