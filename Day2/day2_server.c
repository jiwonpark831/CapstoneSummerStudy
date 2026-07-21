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
    char f_name[NAME_SIZE];
};

int main(int argc, char *argv[])
{
    int serv_sock;
    int str_len;
    socklen_t clnt_adr_sz;
    struct Pkt receive_pkt;
    struct Pkt send_pkt;
    int sequence = 0;
    time_t end_time;
    double spend_time;
    int size = 0;
    FILE *fp;
    int res;
    char buf[2048];

    struct sockaddr_in serv_adr, clnt_adr;
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
        error_handling("UDP socket creation error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);

        // str_len = recvfrom(serv_sock, &receive_pkt, sizeof(receive_pkt), 0,
        //                    (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        // printf("buf:%s", receive_pkt.msg);
        fp = fopen("result.txt", "w");
        if (fp == NULL)
        {
            printf("Failed to open file\n");
            return 0;
        }

        if (0 < (str_len = recvfrom(serv_sock, &receive_pkt, sizeof(receive_pkt), 0,
                                    (struct sockaddr *)&clnt_adr, &clnt_adr_sz)))
        {
            fwrite(receive_pkt.msg, 1, str_len, fp);
        }
        else
        {
            break;
        }
        end_time = time(NULL);

        printf("%s\n", receive_pkt.msg);
        memset(receive_pkt.msg, 0, sizeof(receive_pkt.msg));
        // fprintf(fp, "%s", receive_pkt.msg);

        printf("[Receive PCK from Sender] type: %s, sequence: %d\n", (receive_pkt.type ? "ACK" : "PCK"), receive_pkt.seq);

        if (str_len > 0)
        {
            send_pkt.seq = receive_pkt.seq;
            send_pkt.type = 1;

            sendto(serv_sock, &send_pkt, sizeof(send_pkt), 0,
                   (struct sockaddr *)&clnt_adr, clnt_adr_sz);
            printf("[Send ACK to Sender] type: %s, sequence: %d\n", (send_pkt.type ? "ACK" : "PCK"), send_pkt.seq);
        }

        size += sizeof(receive_pkt.type);
        size += sizeof(receive_pkt.seq);
        size += strlen(receive_pkt.msg);
        // size += strlen(receive_pkt.f_name);
        spend_time = difftime(end_time, receive_pkt.s_time);
        printf("[Result] size: %d, time: %.2f\t %.2f dps\n\n\n", size, spend_time, (size / spend_time));
        size = 0;
        // printf("%d\n", sequence);
        // sequence++;
        // memset(buf, 0, sizeof(buf));

        // printf("%d\n", sequence);
    }
    close(serv_sock);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
