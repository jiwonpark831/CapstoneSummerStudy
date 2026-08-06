#include "p2p.h"

struct Peer_send
{
    int sd;
    int my_idx;
    char **final_buffer;
};

void receiver(char *argv[])
{
    // sender랑 연결
    int receive_sd;
    struct sockaddr_in sender_adr;

    // p2p listen으로 연결
    int receive_p2p_accept_sd, receive_p2p_request_accpet_sd;
    struct sockaddr_in receive_p2p_accept_sd_adr, receive_p2p_request_accpet_sd_adr;
    int receive_p2p_request_accpet_sd_adr_sz;

    // p2p connect로 연결
    int receive_p2p_connect_sd;
    struct sockaddr_in receive_p2p_sender_adr;

    pthread_t send_thread[max_receiver];
    pthread_t receive_thread[max_receiver];
    int connect_flag;   // 다 connect 되어있는지 확인을 위한 flag
    int write_flag = 0; // 다 받아온 후 이제 write하라는 flag
    int my_index;
    char receive_buffer[seg_count][seg_size];
    int cur_cnt = 0;
    int receive_cnt;
    char tmp_buf[1000];
    struct Peer_send peer_info;
    char final_buffer[seg_count][seg_size];
    int receiver_adrs[10];
    short receiver_ports[10];
    int tmp_adr;
    short tmp_port;
    int send_connect_flag;
    int max_receiver;
    int read_cnt;
    int total_read_cnt;

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

    // my_index보다 작은 index 값이면 accept칟
    // my_index보다 큰 애한테 connect
    for (int i = 0; i < seg_count; i++)
    {
        if (i < my_index)
        {
            receive_p2p_request_accpet_sd_adr_sz = sizeof(receive_p2p_request_accpet_sd_adr);
            receive_p2p_request_accpet_sd = accept(receive_p2p_accept_sd, (struct sockaddr *)&receive_p2p_request_accpet_sd_adr, &receive_p2p_request_accpet_sd_adr_sz);
            printf("=[ACCET PEER]\n");

            peer_info.sd = receive_p2p_request_accpet_sd;
            peer_info.my_idx = i;
        }
        else if (my_index < i)
        {
            receive_p2p_connect_sd = socket(PF_INET, SOCK_STREAM, 0);
            if (receive_p2p_connect_sd == -1)
                error_handling("socket() error");

            memset(&receive_p2p_sender_adr, 0, sizeof(receive_p2p_sender_adr));
            receive_p2p_sender_adr.sin_family = AF_INET;
            receive_p2p_sender_adr.sin_addr.s_addr = htonl(receiver_adrs[i]);
            receive_p2p_sender_adr.sin_port = htons(receiver_ports[i]);

            if (connect(receive_p2p_connect_sd, (struct sockaddr *)&receive_p2p_sender_adr, sizeof(receive_p2p_sender_adr)) == -1)
                error_handling("connect() error");
            else
                printf("=[CONNECT TO OTHER PEER]\n");

            peer_info.sd = receive_p2p_connect_sd;
            peer_info.my_idx = i;
            // for (int a = 0; a < seg_count; a++)
            //     peer_info.final_buffer[a] = (char *)malloc(sizeof(char) * seg_size);
            // peer_info.final_buffer = (char **)final_buffer;
        }
    }

    // write로 우리 다 연결됐어 알림
    connect_flag = 1;
    write_all(receive_sd, &connect_flag, sizeof(connect_flag));
    printf("===[PEER CONNECTION DONE]===\n");

    for (int j = 0; j < seg_count; j++)
    {
        pthread_create(&send_thread[j], NULL, peer_send, (void *)&peer_info);
        pthread_create(&receive_thread[j], NULL, peer_receive, (void *)&peer_info);
    }

    // for (int i = 0; i < my_index; i++)
    // {
    //     receive_p2p_request_accpet_sd_adr_sz = sizeof(receive_p2p_request_accpet_sd_adr);
    //     receive_p2p_request_accpet_sd = accept(receive_p2p_accept_sd, (struct sockaddr *)&receive_p2p_request_accpet_sd_adr, &receive_p2p_request_accpet_sd_adr_sz);
    //     printf("=[ACCET PEER]\n");

    //     peer_info.sd = receive_p2p_request_accpet_sd;
    //     peer_info.my_idx = i;
    //     pthread_create(&send_thread[i], NULL, peer_send, (void *)&peer_info);
    //     pthread_create(&receive_thread[i], NULL, peer_receive, (void *)&peer_info);
    // }

    // for (int j = my_index + 1; j < seg_count; j++)
    // {
    //     receive_p2p_connect_sd = socket(PF_INET, SOCK_STREAM, 0);
    //     if (receive_p2p_connect_sd == -1)
    //         error_handling("socket() error");

    //     memset(&receive_p2p_sender_adr, 0, sizeof(receive_p2p_sender_adr));
    //     receive_p2p_sender_adr.sin_family = AF_INET;
    //     receive_p2p_sender_adr.sin_addr.s_addr = htonl(receiver_adrs[j]);
    //     receive_p2p_sender_adr.sin_port = htons(receiver_ports[j]);

    //     if (connect(receive_p2p_connect_sd, (struct sockaddr *)&receive_p2p_sender_adr, sizeof(receive_p2p_sender_adr)) == -1)
    //         error_handling("connect() error");
    //     else
    //         printf("=[CONNECT TO OTHER PEER]\n");

    //     peer_info.sd = receive_p2p_connect_sd;
    //     peer_info.my_idx = j;
    //     for (int a = 0; a < seg_count; a++)
    //         peer_info.final_buffer[a] = (char *)malloc(sizeof(char) * seg_size);
    //     peer_info.final_buffer = (char **)final_buffer;

    //     pthread_create(&send_thread[j], NULL, peer_send, (void *)&peer_info);
    //     pthread_create(&receive_thread[j], NULL, peer_receive, (void *)&peer_info);
    // }

    // sender에게 받은 데이터 read
    for (int k = 0; k < seg_count; k++)
    {
        cur_cnt = 0;
        while (1)
        {
            if (cur_cnt == seg_size)
                break;
            receive_cnt = read(receive_sd, tmp_buf, sizeof(tmp_buf));
            strcat(receive_buffer[k], tmp_buf);
            if (k % seg_count == my_index)
                strcat(final_buffer[k], receive_buffer[k]);
            cur_cnt += receive_cnt;
        }
    }

    write_flag = 1;

    // pthread join
    for (int z = 0; z < seg_count; z++)
    {
        pthread_join(send_thread[z], NULL);
        pthread_join(receive_thread[z], NULL);
    }
}

void *peer_send(void *arg)
{
    int sd = ((struct Peer_send *)arg)->sd;
    int my_idx = ((struct Peer_send *)arg)->my_idx;
    char final_buffer[seg_count][seg_size];
    memcpy(final_buffer, ((struct Peer_send *)arg)->final_buffer, sizeof(((struct Peer_send *)arg)->final_buffer));
    int my_seg_cnt = 0; // 내가 보낼 seg 개수
    struct Pkt send_pkt;

    while (write_flag != 1)
    {
        usleep(10000);
    }

    for (int i = 0; i < seg_count; i++)
        if (i % seg_count == my_idx)
            my_seg_cnt++;

    write(sd, &my_seg_cnt, sizeof(my_seg_cnt));

    for (int i = 0; i < seg_count; i++)
    {
        if (i % seg_count == my_idx)
        {
            // seg index.. size 등등 다, seg 몇개인지
            write(sd, &i, sizeof(i));
            write(sd, &seg_size, sizeof(seg_size));
            if (i = seg_count)
            {
                int last_seg_size = file_size - ((seg_count - 1) * seg_size);
                write(sd, &last_seg_size, sizeof(seg_size));
            }
            else
                write(sd, &seg_size, sizeof(seg_size));

            while (1)
            {
                // write data 1000씩 보내기
                memcpy(send_pkt.data, final_buffer[i], sizeof(final_buffer[i]));
                send_pkt.size = strlen(send_pkt.data);
                write(sd, &send_pkt, sizeof(send_pkt));
                // time 측정을 위한 로직 필요!!!

                // 진행도 출력
                print_sender_result();
            }
        }
    }
}

void *peer_receive(void *arg)
{
    int sd = ((struct Peer_send *)arg)->sd;
    char final_buffer[seg_count][seg_size];
    memcpy(final_buffer, ((struct Peer_send *)arg)->final_buffer, sizeof(((struct Peer_send *)arg)->final_buffer));
    struct Pkt receive_pkt;
    int my_seg_cnt = 0; // 내가 보낼 seg 개수
    char tmp_buf[1000];
    int seg_size;
    int read_cnt;
    int cur_cnt = 0;
    int i;

    read(sd, &my_seg_cnt, sizeof(my_seg_cnt));
    read(sd, &i, sizeof(i));
    for (int j = 0; j < seg_count; j++)
    {
        if (j % seg_count == i)
        {
            cur_cnt = 0;

            read(sd, &seg_size, sizeof(seg_size));
            while (1)
            {
                if (cur_cnt == seg_size)
                    break;
                read_cnt = read(sd, tmp_buf, sizeof(tmp_buf));
                strcat(final_buffer[i], tmp_buf);
                cur_cnt += read_cnt;
            }
        }
    }
}

void print_receiver_result()
{
    printf("print_receiver_result\n");
}
