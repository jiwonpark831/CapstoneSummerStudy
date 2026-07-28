#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>

#define BUF_SIZE 1024
void error_handling(char *message);

int main(int argc, char *argv[])
{
    int sock;
    int str_len, recv_len, recv_cnt, cnt, read_len;
    struct sockaddr_in serv_adr;
    FILE *fp;
    int mode;
    int size;
    char buf[BUF_SIZE];
    char filename[64];
    struct stat file_stat;

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("connect() error!");
    else
        puts("Connected...........");

    while (1)
    {
        memset(buf, 0, sizeof(buf));
        memset(filename, 0, sizeof(filename));

        printf("\n\n===Choose Menu===\n1. Check current path\n2. Move to other dir(cd)\n3. Read file list in current dir(ls)\n4. Download\n5. Upload\n>  ");
        scanf("%d", &mode);

        if (mode == 0)
            break;

        write(sock, &mode, sizeof(mode));
        printf("write mode: %d\n", mode);

        switch (mode)
        {
        case 1: // pwd
            read(sock, buf, BUF_SIZE);
            printf("pwd: %s\n\n", buf);
            memset(buf, 0, sizeof(buf));
            break;
        case 2: // cd
            printf("cd > ");
            scanf("%s", buf);
            write(sock, &buf, sizeof(buf));
            memset(buf, 0, sizeof(buf));
            read(sock, buf, BUF_SIZE);
            printf("%s\n\n", buf);
            memset(buf, 0, sizeof(buf));

            break;
        case 3: // ls
            read(sock, buf, BUF_SIZE);
            printf("%s\n\n", buf);
            memset(buf, 0, sizeof(buf));
            break;
        case 4: // download
            memset(filename, 0, sizeof(filename));
            memset(buf, 0, sizeof(buf));
            printf("Download file name > ");
            scanf("%s", filename);
            filename[strlen(filename)] = '\0';
            write(sock, filename, sizeof(filename));

            read(sock, &size, sizeof(size));
            printf("size: %d\n", size);

            fp = fopen(filename, "wb");

            str_len = 0;

            while ((str_len < size) && (read_len = read(sock, buf, sizeof(buf))))
            {
                if (read_len == -1)
                {
                    error_handling("read() error!");
                    break;
                }
                buf[read_len] = '\0';
                // printf("=======%s", buf);
                fwrite((void *)buf, 1, read_len, fp);
                str_len += read_len;
                if (str_len >= size)
                {
                    break;
                }
            }

            memset(buf, 0, sizeof(buf));
            memset(filename, 0, sizeof(filename));
            printf("download done!\n\n");

            fclose(fp);

            break;
        case 5: // upload
            memset(filename, 0, sizeof(filename));
            memset(buf, 0, sizeof(buf));

            printf("Upload file name > ");
            scanf("%s", filename);

            fp = fopen(filename, "rb");
            printf("%s\n", filename);

            if (fp == NULL)
                printf("Cannot find file %s\n", filename);

            filename[strlen(filename)] = '\0';
            write(sock, filename, sizeof(filename));

            if (stat(filename, &file_stat) == 0)
            {
                size = (int)file_stat.st_size;
                write(sock, &size, sizeof(size));
            }
            memset(filename, 0, sizeof(filename));
            memset(buf, 0, sizeof(buf));

            while (1)
            {
                cnt = fread((void *)buf, 1, sizeof(buf), fp);
                if (cnt < BUF_SIZE)
                {
                    buf[cnt] = '\0';
                    write(sock, buf, cnt);
                    // printf("=====%s", buf);
                    break;
                }
                buf[cnt] = '\0';
                write(sock, buf, sizeof(buf));
                // printf("=====%s", buf);

                str_len += cnt;
                if (str_len >= size)
                {
                    break;
                }
            }
            memset(filename, 0, sizeof(filename));
            memset(buf, 0, sizeof(buf));
            fclose(fp);
            printf("write to server\n\n");

            break;

        default:
            printf("Select Mode\n");
            break;
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
