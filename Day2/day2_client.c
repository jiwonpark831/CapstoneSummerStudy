// tc 테스트 못함
// file name을 안보내서 수정해서 file write 다시 봐야함
// 버퍼랑 문자 처리 디테일 다시 봐야함

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define BUF_SIZE 256
#define NAME_SIZE 30

void error_handling(char *message);

struct Pkt
{
    int seq;
    int type; // 0이면 ptk, 1이면 ack
    char msg[BUF_SIZE];
    time_t s_time;
    // char f_name[NAME_SIZE];
};

int main(int argc, char *argv[])
{
    int sock;
    int str_len;
    socklen_t adr_sz;
    struct Pkt send_pkt;
    struct Pkt receive_pkt;
    struct timeval optVal = {5, 0};
    int optLen = sizeof(optVal);
    int sequence = 0;
    char message[BUF_SIZE];
    time_t start_time;
    FILE *fp;
    int res;

    char file_name[NAME_SIZE];

    struct sockaddr_in serv_adr, from_adr;
    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &optVal, optLen);

    while (1)
    {
        memset(file_name, 0, sizeof(file_name));
        printf("Insert file name(q to quit) > ");
        scanf("%s", file_name);
        if (!strcmp(file_name, "q\n") || !strcmp(file_name, "Q\n"))
            break;
        fp = fopen(file_name, "rb");
        if (fp != NULL)
        {
            if ((str_len != -1))
            {
                while (res = fread(message, sizeof(char), BUF_SIZE, fp))
                {
                    strcpy(send_pkt.msg, message);
                    // printf("%s\n", send_pkt.msg);
                    send_pkt.seq = sequence++;
                    send_pkt.type = 0;
                    // strcpy(send_pkt.f_name, file_name);
                    // printf("file_name: %s\n", send_pkt.f_name);
                    start_time = time(NULL);
                    send_pkt.s_time = start_time;
                    sendto(sock, &send_pkt, sizeof(send_pkt), 0,
                           (struct sockaddr *)&serv_adr, sizeof(serv_adr));
                    printf("[Send PCK to Receiver] type: %s, sequence: %d\n", (send_pkt.type ? "ACK" : "PCK"), send_pkt.seq);

                    memset(send_pkt.msg, 0, sizeof(send_pkt.msg));
                    memset(file_name, 0, sizeof(file_name));

                    adr_sz = sizeof(from_adr);
                    str_len = recvfrom(sock, &receive_pkt, sizeof(receive_pkt), 0,
                                       (struct sockaddr *)&from_adr, &adr_sz);
                    if ((receive_pkt.type == 1))
                    {
                        printf("[Receive ACK from Receiver] type: %s, sequence: %d\n\n\n", (receive_pkt.type ? "ACK" : "PCK"), receive_pkt.seq);
                    }
                }
            }
            else
            {
                printf("== SEND PKT ONE MORE TIME.. ==\n");
                sendto(sock, &send_pkt, sizeof(send_pkt), 0,
                       (struct sockaddr *)&serv_adr, sizeof(serv_adr));
                printf("[Send PCK to Receiver] type: %s, sequence: %d\n", (send_pkt.type ? "ACK" : "PCK"), send_pkt.seq);

                adr_sz = sizeof(from_adr);
                str_len = recvfrom(sock, &receive_pkt, sizeof(receive_pkt), 0,
                                   (struct sockaddr *)&from_adr, &adr_sz);
                if ((receive_pkt.type == 1))
                {
                    printf("[Receive ACK from Receiver] type: %s, sequence: %d\n\n\n", (receive_pkt.type ? "ACK" : "PCK"), receive_pkt.seq);
                }
            }
            fclose(fp);
        }
        else
        {
            printf("Cannot find %s\n", file_name);
            return 0;
        }
    }
    close(sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}