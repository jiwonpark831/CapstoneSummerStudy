#include "p2p.h"

struct Peer_send
{
    int sd;
    int my_idx;
    int peer_idx;
    char **final_buffer;
};

void receiver(char *argv[])
{
    // sender랑 연결
    int receive_sd;
    struct sockaddr_in sender_adr;

    // p2p listen으로 연결
    int receive_p2p_accept_sd, receive_p2p_request_accpet_sd;
    int receive_p2p_request_accpet_sds[max_receiver];
    struct sockaddr_in receive_p2p_accept_sd_adr, receive_p2p_request_accpet_sd_adr;
    int receive_p2p_request_accpet_sd_adr_sz;

    // p2p connect로 연결
    int receive_p2p_connect_sd;
    struct sockaddr_in receive_p2p_sender_adr;

    pthread_t send_thread[10];
    pthread_t receive_thread[10];
    int connect_flag; // 다 connect 되어있는지 확인을 위한 flag
    int my_index;
    char **receive_buffer;
    int cur_cnt = 0;
    int receive_cnt;
    char tmp_buf[1000];
    struct Peer_send peer_info;
    char **write_buffer;
    int receiver_adrs[10];
    short receiver_ports[10];
    int tmp_adr;
    short tmp_port;
    int send_connect_flag;
    int read_cnt;
    int total_read_cnt;
    int cur_seg_size;
    int thread_idx = 0;

    pthread_mutex_lock(&mutex);
    start_flag = 0;
    total_bytes = 0;
    memset(each_bytes, 0, sizeof(each_bytes));
    pthread_mutex_unlock(&mutex);

    // p2p connect를 위한 listen
    receive_p2p_accept_sd = socket(PF_INET, SOCK_STREAM, 0);

    memset(&receive_p2p_accept_sd_adr, 0, sizeof(receive_p2p_accept_sd_adr));
    receive_p2p_accept_sd_adr.sin_family = AF_INET;
    receive_p2p_accept_sd_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    receive_p2p_accept_sd_adr.sin_port = htons(atoi(argv[2]));

    if (bind(receive_p2p_accept_sd, (struct sockaddr *)&receive_p2p_accept_sd_adr, sizeof(receive_p2p_accept_sd_adr)) == -1)
        error_handling("bind() error");
    if (listen(receive_p2p_accept_sd, 10) == -1)
        error_handling("listen() error");

    // sender랑 connect
    receive_sd = socket(PF_INET, SOCK_STREAM, 0);
    if (receive_sd == -1)
        error_handling("socket() error");

    memset(&sender_adr, 0, sizeof(sender_adr));
    sender_adr.sin_family = AF_INET;
    sender_adr.sin_addr.s_addr = inet_addr(argv[5]);
    sender_adr.sin_port = htons(atoi(argv[6]));

    if (connect(receive_sd, (struct sockaddr *)&sender_adr, sizeof(sender_adr)) == -1)
        error_handling("connect() error");
    else
        puts("[CONNECT TO SENDER]");

    read_exact(receive_sd, &max_receiver, sizeof(max_receiver));
    read_exact(receive_sd, &seg_count, sizeof(seg_count));
    read_exact(receive_sd, &seg_size, sizeof(seg_size));
    read_exact(receive_sd, &file_size, sizeof(file_size));

    printf("max_receiver :%d\n", max_receiver);

    // PORT write
    if ((strcmp(argv[1], "-p")) == 0)
    {
        short port = (short)atoi(argv[2]);
        write_all(receive_sd, &port, sizeof(port));
        printf("=send my port: %d\n", port);
    }

    // 내 idx 받아오기
    read_exact(receive_sd, &my_index, sizeof(my_index));
    printf("=read my index: %d\n", my_index);

    // 다른 receiver 정보 받아오기
    memset(receiver_adrs, 0, sizeof(receiver_adrs));
    memset(receiver_ports, 0, sizeof(receiver_ports));
    read_exact(receive_sd, receiver_adrs, sizeof(int) * max_receiver);

    read_exact(receive_sd, receiver_ports, sizeof(short) * max_receiver);

    for (int j = 0; j < max_receiver; j++)
    {
        // read(receive_sd, &receiver_ports[j], sizeof(receiver_ports[j]));
        printf("=read other receiver ip: %d\n", receiver_adrs[j]);
        printf("=read other receiver port: %d\n", receiver_ports[j]);
    }

    receive_buffer = (char **)malloc(sizeof(char *) * seg_count);
    for (int a = 0; a < seg_count; a++)
    {
        receive_buffer[a] = (char *)malloc(sizeof(char) * seg_size);
        memset(receive_buffer[a], 0, seg_size);
    }

    write_buffer = (char **)malloc(sizeof(char *) * seg_count);
    for (int a = 0; a < seg_count; a++)
    {
        write_buffer[a] = (char *)malloc(sizeof(char) * seg_size);
        memset(write_buffer[a], 0, seg_size);
    }

    pthread_mutex_lock(&mutex);
    down_bytes = 0;
    pthread_mutex_unlock(&mutex);

    // my_index보다 작은 index 값이면 accept칟
    if (my_index != 0)
    {
        printf("try to aceept smaller sd\n");
        for (int i = 0; i < my_index; i++)
        {
            // printf("===try to aceept smaller sd\n");

            receive_p2p_request_accpet_sd_adr_sz = sizeof(receive_p2p_request_accpet_sd_adr);
            // printf("===== try to aceept smaller sd\n");
            printf("%d\n", receive_p2p_request_accpet_sd_adr_sz);

            int accept_sd = accept(receive_p2p_accept_sd, (struct sockaddr *)&receive_p2p_request_accpet_sd_adr, &receive_p2p_request_accpet_sd_adr_sz);
            // printf("========try to aceept smaller sd\n");

            printf("=[ACCET PEER]\n");

            struct Peer_send *send_tmp;
            send_tmp = (struct Peer_send *)malloc(sizeof(struct Peer_send));
            struct Peer_send *receive_tmp;
            receive_tmp = (struct Peer_send *)malloc(sizeof(struct Peer_send));

            send_tmp->sd = accept_sd;
            send_tmp->my_idx = my_index;
            send_tmp->peer_idx = i;
            send_tmp->final_buffer = receive_buffer;
            receive_tmp->sd = accept_sd;
            receive_tmp->my_idx = my_index;
            receive_tmp->peer_idx = i;
            receive_tmp->final_buffer = receive_buffer;
            pthread_create(&send_thread[thread_idx], NULL, peer_send, (void *)send_tmp);
            pthread_create(&receive_thread[thread_idx], NULL, peer_receive, (void *)receive_tmp);
            thread_idx++;
        }
    }
    // my_index보다 큰 애한테 connect
    if (my_index != max_receiver - 1)
    {
        printf("try to connect larger sd\n");
        for (int j = my_index + 1; j < max_receiver; j++)
        {
            receive_p2p_connect_sd = socket(PF_INET, SOCK_STREAM, 0);
            if (receive_p2p_connect_sd == -1)
                error_handling("socket() error");

            memset(&receive_p2p_sender_adr, 0, sizeof(receive_p2p_sender_adr));
            receive_p2p_sender_adr.sin_family = AF_INET;
            receive_p2p_sender_adr.sin_addr.s_addr = receiver_adrs[j];
            receive_p2p_sender_adr.sin_port = htons(receiver_ports[j]);

            if (connect(receive_p2p_connect_sd, (struct sockaddr *)&receive_p2p_sender_adr, sizeof(receive_p2p_sender_adr)) == -1)
                error_handling("connect() error");
            else
                printf("=[CONNECT TO OTHER PEER]\n");

            struct Peer_send *send_tmp;
            send_tmp = (struct Peer_send *)malloc(sizeof(struct Peer_send));
            struct Peer_send *receive_tmp;
            receive_tmp = (struct Peer_send *)malloc(sizeof(struct Peer_send));

            send_tmp->sd = receive_p2p_connect_sd;
            send_tmp->my_idx = my_index;
            send_tmp->peer_idx = j;
            send_tmp->final_buffer = receive_buffer;

            receive_tmp->sd = receive_p2p_connect_sd;
            receive_tmp->my_idx = my_index;
            receive_tmp->peer_idx = j;
            receive_tmp->final_buffer = receive_buffer;

            pthread_create(&send_thread[thread_idx], NULL, peer_send, (void *)send_tmp);
            pthread_create(&receive_thread[thread_idx], NULL, peer_receive, (void *)receive_tmp);
            thread_idx++;
        }
    }

    connect_flag = 1;
    write_all(receive_sd, &connect_flag, sizeof(connect_flag));
    printf("===[PEER CONNECTION DONE]===\n");

    // sender에게 받은 데이터 read
    for (int k = 0; k < seg_count; k++)
    {
        if (k % max_receiver == my_index)
        {
            read_exact(receive_sd, &cur_seg_size, sizeof(cur_seg_size));
            cur_cnt = 0;
            while (1)
            {
                if (cur_cnt >= cur_seg_size)
                    break;
                struct Pkt rcv_pkt;
                read_exact(receive_sd, &rcv_pkt, sizeof(rcv_pkt));
                memcpy(&receive_buffer[k][cur_cnt], rcv_pkt.data, rcv_pkt.size);
                cur_cnt += rcv_pkt.size;
                // pthread_mutex_lock(&mutex);
                // down_bytes += rcv_pkt.size;
                // pthread_mutex_unlock(&mutex);
                print_receiver_result(0, rcv_pkt.size, my_index);
            }
        }
    }
    pthread_mutex_lock(&mutex);
    write_flag = 1;
    pthread_mutex_unlock(&mutex);

    printf("===[SENDER -> RECEIVER READ DONE]===\n");

    // pthread join
    for (int z = 0; z < thread_idx; z++)
    {
        pthread_join(send_thread[z], NULL);
        pthread_join(receive_thread[z], NULL);
    }

    printf("===[PEER -> PEER READ DONE]===\n");

    FILE *final_fp = fopen("result_file.png", "wb");

    int write_size;
    for (int i = 0; i < seg_count; i++)
    {
        if (i == seg_count - 1)
            write_size = file_size - ((seg_count - 1) * seg_size);
        else
            write_size = seg_size;

        fwrite(receive_buffer[i], sizeof(char), write_size, final_fp);
    }

    fclose(final_fp);
    printf("===[WRITE FILE DONE]===\n");

    sleep(10);

    close(receive_p2p_accept_sd);
    close(receive_sd);
    for (int j = 0; j < seg_count; j++)
    {
        free(receive_buffer[j]);
        free(write_buffer[j]);
    }
    free(receive_buffer);
    free(write_buffer);
}

void *peer_send(void *arg)
{
    struct Peer_send info = *(struct Peer_send *)arg;
    free(arg);

    int sd = info.sd;
    int my_idx = info.my_idx;
    char **final_buffer = info.final_buffer;
    int my_seg_cnt = 0; // 내가 보낼 seg 개수
    struct Pkt send_pkt;
    int cur_cnt = 0;
    int write_cnt;
    int size;
    int cur_seg_size;
    int flag = 0;

    for (int i = 0; i < seg_count; i++)
        if (i % max_receiver == my_idx)
            my_seg_cnt++;

    while (1)
    {
        pthread_mutex_lock(&mutex);
        flag = write_flag;
        pthread_mutex_unlock(&mutex);
        if (flag == 1)
            break;
        usleep(10000);
    }

    if (write_all(sd, &my_seg_cnt, sizeof(my_seg_cnt)) < 0)
        return NULL;

    if (my_seg_cnt == 0)
        return NULL;

    for (int i = 0; i < seg_count; i++)
    {
        if (i % max_receiver == my_idx)
        {
            cur_cnt = 0;
            // seg index.. size 등등 다, seg 몇개인지
            if (write_all(sd, &i, sizeof(i)) < 0)
                return NULL;

            if (i == seg_count - 1)
                cur_seg_size = file_size - ((seg_count - 1) * seg_size);
            else
                cur_seg_size = seg_size;
            if (write_all(sd, &cur_seg_size, sizeof(cur_seg_size)) < 0)
                return NULL;

            while (1)
            {
                if (cur_cnt >= cur_seg_size)
                    break;

                size = cur_seg_size - cur_cnt;
                if (size > 1000)
                    send_pkt.size = 1000;
                else
                    send_pkt.size = size;
                // write data 1000씩 보내기
                memset(send_pkt.data, 0, sizeof(send_pkt.data));
                memcpy(send_pkt.data, &final_buffer[i][cur_cnt], send_pkt.size);
                if (write_all(sd, &send_pkt, sizeof(send_pkt)) < 0)
                    return NULL;
                cur_cnt += send_pkt.size;
                // time 측정을 위한 로직 필요!!!
            }
        }
    }
    return NULL;
}

void *peer_receive(void *arg)
{
    struct Peer_send info = *(struct Peer_send *)arg;
    free(arg);

    int sd = info.sd;
    int my_index = info.my_idx;
    int peer_index = info.peer_idx;
    char **final_buffer = info.final_buffer;
    int my_seg_cnt = 0; // 내가 보낼 seg 개수
    char tmp_buf[1000];
    int seg_size;
    int read_cnt;
    int cur_cnt = 0;
    int i;
    int cur_seg_size;
    int down_rate = 0;
    int flag = 0;

    while (1)
    {
        pthread_mutex_lock(&mutex);
        flag = write_flag;
        pthread_mutex_unlock(&mutex);
        if (flag == 1)
            break;
        usleep(10000);
    }

    if (read_exact(sd, &my_seg_cnt, sizeof(my_seg_cnt)) < 0)
        return NULL;

    if (my_seg_cnt == 0)
        return NULL;

    for (int j = 0; j < my_seg_cnt; j++)
    {
        if (read_exact(sd, &i, sizeof(i)) < 0)
            return NULL;
        if (read_exact(sd, &cur_seg_size, sizeof(cur_seg_size)) < 0)
            return NULL;
        cur_cnt = 0;

        while (1)
        {
            if (cur_cnt >= cur_seg_size)
                break;

            struct Pkt receive_pkt;

            if (read_exact(sd, &receive_pkt, sizeof(receive_pkt)) < 0)
                return NULL;
            memcpy(&final_buffer[i][cur_cnt], receive_pkt.data, receive_pkt.size);
            cur_cnt += receive_pkt.size;

            print_receiver_result(peer_index + 1, receive_pkt.size, my_index);
            // pthread_mutex_lock(&mutex);

            // down_bytes += receive_pkt.size;
            // printf("\x1b[3J\x1b[2J\x1b[H");
            // down_rate = (int)(((double)down_bytes / file_size) * 100);
            // printf("Receiver from Peer (progress %d%%): (%d/%d)\n",
            //        down_rate, down_bytes, file_size);
            // pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}

void print_receiver_result(int idx, int bytes, int my_idx)
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
    percent = (int)(((double)total_bytes / file_size) * 100);
    bps = (total_bytes * 8.0) / (diffTime * 1000000.0);
    int cnt = (percent * 25) / 100;
    printf("\x1b[3J\x1b[2J\x1b[H");
    printf("Receiving Peer %d [", my_idx + 1);
    for (int i = 0; i < cnt; i++)
        printf("#");
    for (int j = cnt; j < 25; j++)
        printf(" ");
    printf("] %d%% (%d/%dBytes) %.2fMbps (%.2fs)\n", percent, total_bytes, file_size, bps, diffTime);
    double sender_bps = (each_bytes[0] * 8.0) / (diffTime * 1000000.0);
    printf("From Sending Peer : %.2fMbps (%d Bytes Sent / %.2fs)\n", sender_bps, each_bytes[0], diffTime);

    for (int z = 0; z < max_receiver; z++)
    {
        if (z == my_idx)
            continue;
        double my_bps = (each_bytes[z + 1] * 8.0) / (diffTime * 1000000.0);
        printf("From Receiving Peer #%d : %.2fMbps (%d Bytes Sent / %.2fs)\n", z + 1, my_bps, each_bytes[z + 1], diffTime);
    }
    pthread_mutex_unlock(&mutex);
}
