#include "p2p.h"

struct Peer_send_info
{
    int sd;
    int my_idx;
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
    struct Receiver_info other_receiver_info;
    int my_index;
    char receive_buffer[seg_count][seg_size];
    int cur_cnt = 0;
    int receive_cnt;
    char tmp_buf[1000];
    struct Peer_send_info peer_send_info;

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
        puts("Connected..................");

    // PORT write
    write(receive_sd, argv[2], sizeof(argv[2])); // htons(atoi(argv[2]))

    // 내 idx, 다른 receiver 정보, seg info 등 받아오기
    read(receive_sd, &my_idx, sizeof(my_idx));

    // 몇명인지 읽고.. rea_cnt +=
    read(receive_sd, &other_receiver_info, sizeof(other_receiver_info));

    // my_index보다 작은 index 값이면 accept
    for (int i = 0; i < my_index; i++)
    {

        receive_p2p_request_accpet_sd_adr_sz = sizeof(receive_p2p_request_accpet_sd_adr);
        receive_p2p_request_accpet_sd = accept(receive_p2p_accept_sd, (struct sockaddr *)&receive_p2p_request_accpet_sd_adr, &receive_p2p_request_accpet_sd_adr_sz);

        peer_send_info.sd = receive_p2p_request_accpet_sd;
        peer_send_info.my_idx = i;
        pthread_create(&send_thread[i], NULL, peer_send, (void *)&peer_send_info);
        pthread_create(&receive_thread[i], NULL, peer_receive, (void *)&receive_p2p_request_accpet_sd);
    }

    // my_index보다 큰 애한테 connect
    for (int j = my_index + 1; j < seg_count; j++)
    {
        receive_p2p_connect_sd = socket(PF_INET, SOCK_STREAM, 0);
        if (receive_p2p_connect_sd == -1)
            error_handling("socket() error");

        memset(&receive_p2p_sender_adr, 0, sizeof(receive_p2p_sender_adr));
        receive_p2p_sender_adr.sin_family = AF_INET;
        receive_p2p_sender_adr.sin_addr.s_addr = inet_addr(other_receiver_info[j].ip);
        receive_p2p_sender_adr.sin_port = htons(atoi(other_receiver_info[j].port));

        if (connect(receive_p2p_connect_sd, (struct sockaddr *)&receive_p2p_sender_adr, sizeof(receive_p2p_sender_adr)) == -1)
            error_handling("connect() error");
        else
            puts("Connected..................");

        peer_send_info.sd = receive_p2p_connect_sd;
        peer_send_info.my_idx = j;
        pthread_create(&send_thread[j], NULL, peer_send, (void *)&peer_send_info);
        pthread_create(&receive_thread[j], NULL, peer_receive, (void *)&receive_p2p_connect_sd);
    }

    // write로 우리 다 연결됐어! 가 필요
    connect_flag = 1;
    write(receive_sd, &connect_flag, sizeof(connect_flag));

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
    int sd = *((struct *)arg).sd;
    int my_idx = *((struct *)arg).my_idx;
    while (write_flag != 1)
    {
        usleep(10000);
    }

    for (int i = 0; i < seg_count; i++)
    {

        if (i % seg_count == my_idx)
            // seg index.. size 등등 다, seg 몇개인지
            write(arg, &i, sizeof(i));
    }
}

void *peer_receive(void *arg)
{

    while (1)
    {
        // read();
    }
}

void print_receiver_result()
{
    printf("print_receiver_result\n");
}