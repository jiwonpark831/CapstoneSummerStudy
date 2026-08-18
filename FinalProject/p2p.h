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
#include <sys/time.h>

struct Pkt
{
    char data[1000];
    int size; // data의 size
};

void sender(char *argv[]);
void *sender_send(void *arg);
void receiver(char *argv[]);
void *peer_send(void *arg);
void *peer_receive(void *arg);
void error_handling(char *msg);
void opt_setting(char *argv[]);
void print_sender_result(int idx, int bytes);
void print_receiver_result(int idx, int bytes, int my_idx);
int read_exact(int fd, void *buf, size_t len);
int write_all(int fd, const void *buf, size_t len);

extern int max_receiver;   // -n: 최대 receiving peer수
extern char file_name[20]; // -f: 보낼 파일 이름
extern int seg_size;       // -g: segment 크기, 단위는 KB

extern long long file_size; // file 총 사이즈
extern int seg_count;       // segment 개수
extern int receiver_sds[10];
extern int receiver_adrs[10];
extern short receiver_ports[10];

// extern int write_flag; // 다 받아온 후 이제 write하라는 flag

extern struct timeval startTime;
extern long long each_bytes[10];
extern long long total_bytes;
extern int start_flag;
extern int write_flags[100];

extern pthread_mutex_t mutex;

#endif