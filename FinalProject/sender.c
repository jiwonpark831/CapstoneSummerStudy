#include "p2p.h"

struct Sender_send_info
{
    int sd;
    int my_idx;
    char **fread_buffer;
};

void sender(char *argv[])
{
    int sender_sd, receiver_sd;
    struct sockaddr_in sender_adr, receiver_adr;
    int receiver_adr_sz;
    struct stat file_stat;
    int receiver_idx = 0;
    pthread_t tid[max_receiver];
    short tmp_port;
    FILE *fp;
    int fread_cnt;
    int cur_cnt = 0;
    char tmp_buf[seg_size];
    char **fread_buffer;
    int connect_flag;

    memset(receiver_sds, 0, sizeof(receiver_sds));
    memset(receiver_adrs, 0, sizeof(receiver_adrs));
    memset(receiver_ports, 0, sizeof(receiver_ports));

    // file size 구하기
    if (stat(file_name, &file_stat) == 0)
    {
        file_size = (int)file_stat.st_size;
        printf("\n=SET GLOBAL VAR= File size: %lld\n", file_size);
    }

    if (file_size == 0)
    {
        printf("File size is 0\n");
        exit(1);
    }

    // segment 개수 구하기
    if (file_size % seg_size == 0)
        seg_count = file_size / seg_size;
    else
        seg_count = (file_size / seg_size) + 1;

    printf("=SET GLOBAL VAR= Segment count: %d\n", seg_count);

    sender_sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&sender_adr, 0, sizeof(sender_adr));
    sender_adr.sin_family = AF_INET;
    sender_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    sender_adr.sin_port = htons(atoi(argv[2]));

    // accept
    if (bind(sender_sd, (struct sockaddr *)&sender_adr, sizeof(sender_adr)) == -1)
        error_handling("bind() error");
    if (listen(sender_sd, 5) == -1)
        error_handling("listen() error");

    printf("\n\nListening.....\n\n");

    // fread 돌면서 segment 채우기
    fp = fopen(file_name, "rb");
    fread_buffer = (char **)malloc(sizeof(char *) * seg_count);
    for (int a = 0; a < seg_count; a++)
    {
        fread_buffer[a] = (char *)malloc(sizeof(char) * seg_size);
        memset(fread_buffer[a], 0, seg_size);
    }

    for (int i = 0; i < seg_count; i++)
    {
        cur_cnt = 0;
        while (1)
        {
            memset(tmp_buf, 0, sizeof(tmp_buf));
            if (cur_cnt >= seg_size)
                break;
            fread_cnt = fread((void *)tmp_buf, sizeof(char), seg_size, fp);
            if (fread_cnt < seg_size)
            {
                tmp_buf[fread_cnt] = '\0';
                memcpy(&fread_buffer[i][cur_cnt], tmp_buf, fread_cnt);
                // strcat(fread_buffer[i], tmp_buf);
                memset(tmp_buf, 0, sizeof(tmp_buf));
                printf("=DEBUG= fread count: %d\ni: %d\n", fread_cnt, i);

                break;
            }
            tmp_buf[fread_cnt] = '\0';
            memcpy(&fread_buffer[i][cur_cnt], tmp_buf, fread_cnt);
            printf("=DEBUG= fread count: %d\ni: %d\n", fread_cnt, i);

            cur_cnt += fread_cnt;
            printf("=DEBUG= curr cnt: %d\n", cur_cnt);
            memset(tmp_buf, 0, sizeof(tmp_buf));
        }
    }
    printf("[FREAD DONE]\n\n");
    fclose(fp);

    while (1)
    {
        if (max_receiver == receiver_idx)
            break;

        receiver_adr_sz = sizeof(receiver_adr);
        receiver_sd = accept(sender_sd, (struct sockaddr *)&receiver_adr, &receiver_adr_sz);
        printf("[ACCEPT]\n");
        write_all(receiver_sd, &max_receiver, sizeof(max_receiver));
        write_all(receiver_sd, &seg_count, sizeof(seg_count));
        write_all(receiver_sd, &seg_size, sizeof(seg_size));
        write_all(receiver_sd, &file_size, sizeof(file_size));

        pthread_mutex_lock(&mutex);
        printf("=idx= %d\n", receiver_idx);
        receiver_sds[receiver_idx] = receiver_sd;
        printf("receiver_sds[receiver_idx] : %d\n", receiver_sds[receiver_idx]);
        receiver_adrs[receiver_idx] = receiver_adr.sin_addr.s_addr;
        printf("receiver_adrs[receiver_idx] : %d\n", receiver_adrs[receiver_idx]);
        pthread_mutex_unlock(&mutex);

        // receivers ip, port 저장
        read_exact(receiver_sds[receiver_idx], &tmp_port, sizeof(tmp_port));
        printf("=port: %d", tmp_port);
        receiver_ports[receiver_idx] = tmp_port;
        printf("[RECEIVER ACCEPT] %d, %d\n", receiver_adrs[receiver_idx], receiver_ports[receiver_idx]);

        pthread_mutex_lock(&mutex);
        receiver_idx++;
        pthread_mutex_unlock(&mutex);
    }

    printf("[CONNECT WITH RECEIVER DONE]\n\n");

    for (int i = 0; i < receiver_idx; i++)
    {
        write_all(receiver_sds[i], &i, sizeof(i));

        write_all(receiver_sds[i], receiver_adrs, sizeof(int) * receiver_idx);
        write_all(receiver_sds[i], receiver_ports, sizeof(short) * receiver_idx);

        for (int j = 0; j < max_receiver; j++)
        {
            // write(receiver_sds[i], &receiver_adrs[j], sizeof(receiver_adrs[j]));
            printf("=write ip to recevier: %d\n", receiver_adrs[j]);
            // write(receiver_sds[i], &receiver_ports[j], sizeof(receiver_ports[j]));
            printf("=write port to recevier: %d\n", receiver_ports[j]);
        }

        // 다 connect 되어있는지 확인
        read_exact(receiver_sds[i], &connect_flag, sizeof(connect_flag));

        while (connect_flag != 1)
            usleep(10000);

        printf("===[PEER CONNECTION DONE]===\n");
    }

    struct Sender_send_info *tmp;

    for (int t = 0; t < receiver_idx; t++)
    {
        tmp = (struct Sender_send_info *)malloc(sizeof(struct Sender_send_info));
        tmp->sd = receiver_sds[t];
        tmp->my_idx = t;
        tmp->fread_buffer = (char **)malloc(sizeof(char *) * seg_count);
        for (int a = 0; a < seg_count; a++)
        {
            tmp->fread_buffer[a] = (char *)malloc(sizeof(char) * seg_size);
            memcpy(tmp->fread_buffer[a], fread_buffer[a], seg_size);
        }
        printf("-%d, %d\n", tmp->sd, tmp->my_idx);

        pthread_create(&tid[t], NULL, sender_send, (void *)tmp);
        printf("======pthread_create==========\n");
    }

    for (int y = 0; y < receiver_idx; y++)
        pthread_join(tid[y], NULL);

    close(sender_sd);
}

void *sender_send(void *arg)
{
    struct Sender_send_info info = *(struct Sender_send_info *)arg;
    free(arg);
    int receiver_sock = info.sd;
    int my_idx = info.my_idx;
    char **fread_buffer = info.fread_buffer;
    struct Pkt pkt;
    int cur_cnt = 0;
    int size;
    long long cur_seg_size;

    int cur_idx;
    for (cur_idx = 0; cur_idx < seg_count; cur_idx++)
    {
        // 내것만 보내야함
        if (cur_idx % max_receiver == my_idx)
        {
            printf("DEBUGG==========3\n");

            // segment each size 보내기
            if (cur_idx == seg_count - 1)
                cur_seg_size = file_size - ((long long)(seg_count - 1) * seg_size);
            else
                cur_seg_size = seg_size;
            write_all(receiver_sock, &cur_seg_size, sizeof(cur_seg_size));

            cur_cnt = 0;
            while (1)
            {
                if (cur_cnt >= cur_seg_size)
                    break;
                // write data 1000씩 보내기
                size = cur_seg_size - cur_cnt;
                if (size > 1000)
                    pkt.size = 1000;
                else
                    pkt.size = size;

                memset(pkt.data, 0, sizeof(pkt.data));
                memcpy(pkt.data, &fread_buffer[cur_idx][cur_cnt], pkt.size);
                write_all(receiver_sock, &pkt, sizeof(pkt));
                cur_cnt += pkt.size;

                // 진행도 출력
                print_sender_result(my_idx, pkt.size);
            }
        }
    }
}

void print_sender_result(int idx, int bytes)
{
    struct timeval endTime;
    double diffTime;
    int percent;
    double bps;

    pthread_mutex_lock(&mutex);

    if (start_flag == 0)
    {
        gettimeofday(&startTime, NULL);
        start_flag = 1;
    }

    gettimeofday(&endTime, NULL);
    each_bytes[idx] += bytes;
    total_bytes += bytes;
    diffTime = (endTime.tv_sec - startTime.tv_sec) + ((endTime.tv_usec - startTime.tv_usec) / 1000000.0);
    if (diffTime == 0.0)
        diffTime = 0.1;
    if (file_size > 0)
        percent = (int)(((double)total_bytes / file_size) * 100);
    else
        percent = 100;
    bps = (total_bytes * 8.0) / (diffTime * 1000000.0);
    int cnt = (percent * 25) / 100;
    printf("\x1b[3J\x1b[2J\x1b[H");
    printf("Sending Peer [");
    for (int i = 0; i < cnt; i++)
        printf("#");
    for (int j = cnt; j < 25; j++)
        printf(" ");
    printf("] %d%% (%lld/%lldBytes) %.2fMbps (%.2fs)\n", percent, total_bytes, file_size, bps, diffTime);
    for (int z = 0; z < max_receiver; z++)
    {
        double my_bps = (each_bytes[z] * 8.0) / (diffTime * 1000000.0);
        printf("To Receiving Peer #%d : %.2fMbps (%lld Bytes Sent / %.2fs)\n", z + 1, my_bps, each_bytes[z], diffTime);
    }
    pthread_mutex_unlock(&mutex);
}