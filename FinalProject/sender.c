#include "p2p.h"

void sender(char *argv[])
{
    int sender_sd, receiver_sd;
    struct sockaddr_in sender_adr, receiver_adr;
    int receiver_adr_sz;
    struct stat file_stat;
    pthread_t tid[max_receiver];
    short tmp_port;
    FILE *fp;
    int fread_cnt;
    int cur_cnt = 0;
    char tmp_buf[1000];
    char fread_buffer[seg_count][seg_size];
    int connect_flag;

    // file size 구하기
    if (stat(file_name, &file_stat) == 0)
    {
        file_size = (int)file_stat.st_size;
        printf("File size: %d\n", file_size);
    }

    // segment 개수 구하기
    if (file_size % seg_size == 0)
        seg_count = file_size / seg_size;
    else
        seg_count = (file_size / seg_size) + 1;

    printf("Segment count: %d\n", seg_count);

    sender_sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&sender_sd, 0, sizeof(sender_sd));
    sender_adr.sin_family = AF_INET;
    sender_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    sender_adr.sin_port = htons(atoi(argv[2]));

    // accept
    if (bind(sender_sd, (struct sockaddr *)&sender_adr, sizeof(sender_adr)) == -1)
        error_handling("bind() error");
    if (listen(sender_sd, 5) == -1)
        error_handling("listen() error");

    // fread 돌면서 segment 채우기
    fp = fopen(file_name, "rb");
    for (int i = 0; i < seg_count; i++)
    {
        cur_cnt = 0;
        while (1)
        {
            if (cur_cnt == seg_size)
                break;
            fread_cnt = fread((void *)tmp_buf, 1, sizeof(tmp_buf), fp);
            strcat(fread_buffer[i], tmp_buf);
            cur_cnt += fread_cnt;
        }
    }

    // thread 생성
    while (1)
    {
        if (max_receiver < receiver_idx)
            break;

        receiver_adr_sz = sizeof(receiver_adr);
        receiver_sd = accept(sender_sd, (struct sockaddr *)&receiver_adr, &receiver_adr_sz);

        pthread_mutex_lock(&mutex);
        receiver_sds[receiver_idx++] = receiver_sd;
        pthread_mutex_unlock(&mutex);

        // receivers ip, port 저장
        receiver_info.ip[receiver_idx] = receiver_adr.sin_addr.s_addr;
        read(receiver_sd, &tmp_port, sizeof(tmp_port));
        receiver_info.port[receiver_idx] = tmp_port;

        pthread_create(&tid[receiver_idx], NULL, sender_send, (void *)&receiver_sd);
        pthread_detach(tid[receiver_idx]);
    }
    close(sender_sd);
}

void *sender_send(void *arg)
{
    int receiver_sock = *((int *)arg);
    struct Pkt pkt;
    int connect_flag;

    for (int cur_idx = 0; cur_idx < seg_count; cur_idx++)
    {
        // 내것만 보내야함
        // =======
        if (cur_idx % max_receiver == receiver_idx)
        // =======
        {
            // 다른 receiver들 정보 보내기, 내 idx 보내기
            write(receiver_sds[cur_idx], &cur_idx, sizeof(cur_idx));
            write(receiver_sds[cur_idx], &receiver_info, sizeof(receiver_info));

            // 다 connect 되어있는지 확인
            read(receiver_sds[cur_idx], &connect_flag, sizeof(connect_flag));

            while (connect_flag != 1)
            {
                usleep(10000);
            }

            // segment each size 보내기
            if (cur_idx = seg_count)
            {
                int last_seg_size = file_size - ((seg_count - 1) * seg_size);
                write(receiver_sds[cur_idx], &last_seg_size, sizeof(seg_size));
            }
            else
                write(receiver_sds[cur_idx], &seg_size, sizeof(seg_size));

            while (1)
            {
                // write data 1000씩 보내기
                pkt.data = fread_buffer[cur_idx];
                pkt.size = strlen(pkt.data);
                write(receiver_sds[cur_idx], &pkt, sizeof(pkt));
                // time 측정을 위한 로직 필요!!!

                // 진행도 출력
                print_sender_result();
            }
        }
    }
}

void print_sender_result()
{
    printf("print_sender_result\n");
}