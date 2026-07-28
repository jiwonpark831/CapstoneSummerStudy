#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
#define EPOLL_SIZE 50
void error_handling(char *buf);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;
    int str_len, i, cnt, read_len;
    char buf[BUF_SIZE];
    FILE *fp;
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char pwd[BUF_SIZE];
    char file_path[BUF_SIZE];
    int mode;
    int size;
    char filename[64];
    char size_c[4];

    struct epoll_event *ep_events;
    struct epoll_event event;
    int epfd, event_cnt;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));

    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("bind() error");

    epfd = epoll_create(EPOLL_SIZE);
    ep_events = malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

    while (1)
    {
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        if (event_cnt == -1)
        {
            puts("epoll_wait() error");
            break;
        }
        for (i = 0; i < event_cnt; i++)
        {
            if (ep_events[i].data.fd == serv_sock)
            {
                // server
                adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);

                printf("connected client: %d \n", clnt_sock);
            }
            else
            {
                // client
                memset(buf, 0, sizeof(buf));
                memset(filename, 0, sizeof(filename));

                printf("Loading.....\n\n");

                str_len = read(ep_events[i].data.fd, &mode, sizeof(mode));

                printf("mode: %d\n", mode);
                if (str_len == 0)
                { // close request!
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL);
                    close(ep_events[i].data.fd);
                    printf("closed client: %d \n", ep_events[i].data.fd);
                }
                else
                {
                    printf("mode: %d\n", mode);

                    switch (mode)
                    {
                    case 1: // pwd
                        getcwd(pwd, sizeof(buf));
                        printf("pwd: %s\n", pwd);
                        write(ep_events[i].data.fd, &pwd, sizeof(pwd));
                        memset(pwd, 0, sizeof(pwd));
                        break;
                    case 2: // cd
                        read(ep_events[i].data.fd, file_path, sizeof(file_path));
                        printf("path: %s\n", file_path);
                        chdir(file_path);
                        getcwd(pwd, sizeof(pwd));
                        printf("pwd: %s\n", pwd);
                        dir = opendir(pwd);
                        if (dir == NULL)
                        {
                            printf("Failed to open dir\n");
                            return 0;
                        }
                        printf("move to path: %s\n", file_path);

                        while ((entry = readdir(dir)) != NULL)
                        {
                            strcat(buf, entry->d_name);
                            if (stat(entry->d_name, &file_stat) == 0)
                            {
                                size = (int)file_stat.st_size;
                            }
                            snprintf(size_c, sizeof(size_c), "%d", size);
                            strcat(buf, "\t");
                            strcat(buf, size_c);
                            strcat(buf, "\n");
                        }
                        printf("current dir list: %s\n", buf);
                        closedir(dir);
                        write(ep_events[i].data.fd, buf, sizeof(buf));

                        memset(buf, 0, sizeof(buf));

                        break;
                    case 3: // ls
                        getcwd(pwd, sizeof(pwd));
                        printf("pwd: %s\n", file_path);

                        dir = opendir(pwd);
                        if (dir == NULL)
                        {
                            printf("Failed to open current directory\n");
                            return 0;
                        }

                        while ((entry = readdir(dir)) != NULL)
                        {
                            strcat(buf, entry->d_name);
                            if (stat(entry->d_name, &file_stat) == 0)
                            {
                                size = (int)file_stat.st_size;
                            }
                            snprintf(size_c, sizeof(size_c), "%d", size);
                            strcat(buf, "\t");
                            strcat(buf, size_c);
                            strcat(buf, "\n");
                        }
                        printf("current dir list: %s\n", buf);
                        write(ep_events[i].data.fd, buf, sizeof(buf));

                        memset(buf, 0, sizeof(buf));
                        closedir(dir);
                        break;
                    case 4: // download
                        memset(filename, 0, sizeof(filename));
                        memset(buf, 0, sizeof(buf));

                        read(ep_events[i].data.fd, filename, sizeof(filename));
                        fp = fopen(filename, "rb");
                        if (stat(filename, &file_stat) == 0)
                        {
                            size = (int)file_stat.st_size;
                            printf("size: %d\n", size);
                            write(ep_events[i].data.fd, &size, sizeof(size));
                        }

                        while (1)
                        {
                            cnt = fread((void *)buf, 1, sizeof(buf), fp);
                            if (cnt < BUF_SIZE)
                            {
                                printf("=======%s", buf);
                                write(ep_events[i].data.fd, buf, cnt);
                                break;
                            }
                            printf("=======%s", buf);
                            write(ep_events[i].data.fd, buf, sizeof(buf));
                        }

                        memset(buf, 0, sizeof(buf));
                        memset(filename, 0, sizeof(filename));
                        fclose(fp);
                        break;

                    case 5: // upload
                        memset(filename, 0, sizeof(filename));
                        memset(buf, 0, sizeof(buf));
                        read(ep_events[i].data.fd, filename, sizeof(filename));

                        fp = fopen(filename, "wb");

                        read(ep_events[i].data.fd, &size, sizeof(size));

                        str_len = 0;

                        while ((str_len < size) && (read_len = read(ep_events[i].data.fd, buf, sizeof(buf))))
                        {
                            if (read_len == -1)
                            {
                                error_handling("read() error!");
                                break;
                            }
                            printf("=======%s", buf);

                            fwrite((void *)buf, 1, sizeof(buf), fp);
                            str_len += read_len;
                        }

                        memset(filename, 0, sizeof(filename));
                        memset(buf, 0, sizeof(buf));
                        fclose(fp);
                        break;

                    default:
                        printf("MODE Error\n");
                    }
                }
            }
        }
    }
    close(serv_sock);
    close(epfd);
    return 0;
}

void error_handling(char *buf)
{
    fputs(buf, stderr);
    fputc('\n', stderr);
    exit(1);
}