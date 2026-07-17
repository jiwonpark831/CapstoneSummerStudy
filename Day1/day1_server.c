#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dirent.h>

#define BUF_SIZE 30
#define LIST_SIZE 4
#define MAX 1024
void error_handling(char *message);

int main(int argc, char *argv[])
{
    int serv_sd, clnt_sd;
    FILE *fp;
    char buf[BUF_SIZE];
    int read_cnt;
    struct dirent *entry;
    char file_list[MAX];
    // int file_count = 0;
    int size_int;
    char size[LIST_SIZE];
    char selected_file[MAX] = "";

    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    DIR *dir = opendir(".");
    if (dir == NULL)
    {
        printf("Failed to open dir\n");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, ".", 1) == 0)
            continue;
        strcat(file_list, entry->d_name);
        strcat(file_list, "\n");
        // printf("%s\n", entry->d_name);
        // file_count++;
    }

    // printf("Total num of file: %d\n", file_count);

    // printf("Saved to file_list: %s\n", file_list);

    closedir(dir);

    serv_sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    bind(serv_sd, (struct sockaddr *)&serv_adr, sizeof(serv_adr));
    listen(serv_sd, 5);

    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sd = accept(serv_sd, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

    size_int = strlen(file_list);
    sprintf(size, "%d", size_int);
    write(clnt_sd, size, sizeof(size));
    printf("[Send file list size]\n%s\n", size);

    write(clnt_sd, file_list, strlen(file_list));

    printf("[Send file list]\n%s\n\n\n", file_list);

    read(clnt_sd, selected_file, sizeof(selected_file));
    printf("[Receive select file name]\n%s\n\n\n", selected_file);

    fp = fopen(selected_file, "rb");

    while (1)
    {
        read_cnt = fread((void *)buf, 1, BUF_SIZE, fp);
        if (read_cnt < BUF_SIZE)
        {
            write(clnt_sd, buf, read_cnt);
            break;
        }
        write(clnt_sd, buf, BUF_SIZE);
    }
    printf("!!Send selectd file content!!\n\n\n");

    shutdown(clnt_sd, SHUT_WR);
    read(clnt_sd, buf, BUF_SIZE);
    printf("=== Message from client: %s \n", buf);

    fclose(fp);
    close(clnt_sd);
    close(serv_sd);
    return 0;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
