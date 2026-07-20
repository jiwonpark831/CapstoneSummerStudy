#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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
    int serv_sock;
    int str_len;
    socklen_t clnt_adr_sz;
    struct Pkt receive_pkt;
    struct Pkt send_pkt;
    int sequence = 0;

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

        str_len = recvfrom(serv_sock, &receive_pkt, sizeof(receive_pkt), 0,
                           (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        printf("[Receive PCK from Sender] type: %s, sequence: %d, message: %s", (receive_pkt.type ? "ACK" : "PCK"), receive_pkt.seq, receive_pkt.msg);

        // ACK를 안 받은 걸로 치고 넘어가서 sender가 재전송 보내게 하는 부분
        if ((rand() % 100) < 20)
        {
            printf("== TEST: ignore SENDER'S ACK.. waiting for resending... == \n");
            continue;
        }
        // 여기까지

        if ((str_len > 0) && (receive_pkt.seq == sequence))
        {
            send_pkt.seq = sequence;
            send_pkt.type = 1;
            strcpy(send_pkt.msg, receive_pkt.msg);

            sendto(serv_sock, &send_pkt, sizeof(send_pkt), 0,
                   (struct sockaddr *)&clnt_adr, clnt_adr_sz);
            printf("[Send ACK to Sender] type: %s, sequence: %d, message: %s\n\n\n", (send_pkt.type ? "ACK" : "PCK"), send_pkt.seq, send_pkt.msg);
        }

        // printf("%d\n", sequence);
        sequence++;
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
