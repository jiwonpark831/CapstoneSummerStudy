#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define BUF_SIZE 1024
void error_handling(char *message);

struct Pkt
{
    int seq;
    int type; // 0이면 ptk, 1이면 ack
    char msg[BUF_SIZE];
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
    time_t end_time;
    double spend_time;
    int size = 0;

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
        if ((str_len != -1))
        {
            fputs("Insert message(q to quit) > ", stdout);
            fgets(message, sizeof(message), stdin);
            if (!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
                break;

            strcpy(send_pkt.msg, message);
            send_pkt.seq = sequence++;
            send_pkt.type = 0;

            start_time = time(NULL);
            sendto(sock, &send_pkt, sizeof(send_pkt), 0,
                   (struct sockaddr *)&serv_adr, sizeof(serv_adr));
        }
        else
        {
            printf("== SEND PKT ONE MORE TIME.. ==\n");
            sendto(sock, &send_pkt, sizeof(send_pkt), 0,
                   (struct sockaddr *)&serv_adr, sizeof(serv_adr));
        }

        printf("[Send PCK to Receiver] type: %s, sequence: %d, message: %s", (send_pkt.type ? "ACK" : "PCK"), send_pkt.seq, send_pkt.msg);

        adr_sz = sizeof(from_adr);
        str_len = recvfrom(sock, &receive_pkt, sizeof(receive_pkt), 0,
                           (struct sockaddr *)&from_adr, &adr_sz);
        end_time = time(NULL);
        if ((receive_pkt.type == 1) && (receive_pkt.seq == send_pkt.seq))
        {
            printf("[Receive ACK from Receiver] type: %s, sequence: %d, message: %s", (receive_pkt.type ? "ACK" : "PCK"), receive_pkt.seq, receive_pkt.msg);
            size += sizeof(receive_pkt.type);
            size += sizeof(receive_pkt.seq);
            size += strlen(receive_pkt.msg);
            spend_time = difftime(end_time, start_time);
            printf("[Result] size: %d, time: %.2f\t %.2f dps\n\n\n", size, spend_time, (size / spend_time));
            size = 0;
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