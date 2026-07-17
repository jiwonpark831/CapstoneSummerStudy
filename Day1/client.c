#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 30
#define LIST_SIZE 4
#define MAX 1024
void error_handling(char *message);

int main(int argc, char *argv[])
{
    int sd;
    FILE *fp;
    char buf[BUF_SIZE];
    int read_cnt;
    char file_list[MAX];
    char d_name[BUF_SIZE];
    char size[LIST_SIZE];
    int size_int;
    char selectd_file[MAX];
    int str_len = 0;
    int idx = 0, read_len = 0;

    struct sockaddr_in serv_adr;

    if (argc != 3)
    {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    connect(sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
    printf("Connection done\n");

    while (str_len < LIST_SIZE)
    {
        read_len = read(sd, size, sizeof(size));
        str_len += read_len;
    }
    printf("Receive file size: %s\n", size);

    size_int = atoi(size);
    str_len = 0;
    printf("Convert file size to int: %d\n", size_int);

    // while (str_len < size_int && (read_len = read(sd, &file_list[idx], BUF_SIZE)))
    // {
    // 	if (read_len == -1)
    // 	{
    // 		error_handling("read() error!");
    // 		break;
    // 	}
    // 	printf("%s", file_list);
    // 	idx += read_len;
    // 	str_len += read_len;
    // }
    while ((str_len < size_int) && (read_len = read(sd, d_name, BUF_SIZE)))
    {
        if (read_len == -1)
        {
            error_handling("read() error!");
            break;
        }
        strcat(file_list, d_name);
        str_len += read_len;
    }
    printf("Receive file list: %s\n", file_list);

    printf("Select file > ");
    scanf("%s", selectd_file);
    write(sd, selectd_file, strlen(selectd_file));
    printf("Send selected file\n");

    fp = fopen(selectd_file, "wb");

    while ((read_cnt = read(sd, buf, BUF_SIZE)) != 0)
        fwrite((void *)buf, 1, read_cnt, fp);

    puts("Received file data");
    write(sd, "Thank you", 10);
    fclose(fp);
    close(sd);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}