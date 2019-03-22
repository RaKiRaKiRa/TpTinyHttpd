#include "threadpool.h"
#include "httpd.c"


int main(int argc,char** argv)
{
    int np = argv[1];
    int server_sock = -1;
    u_short port = 4000;
    int client_sock = -1;
    struct sockaddr_in client_name;
    socklen_t  client_name_len = sizeof(client_name);
    pthread_t newthread;

    server_sock = startup(&port);
    printf("httpd running on port %d\n", port);
    struct threadpool *tp = threadpool_init( np );

    while (1)
    {
        client_sock = accept( server_sock, (struct sockaddr *)&client_name, &client_name_len );
        if ( client_sock == -1 )
            error_die( "accept" );
        /* accept_request(&client_sock); */
        if ( threadpool_add_job(tp , (void *)accept_request, (void *)(intptr_t)client_sock) != 0 )//long型 4位字节 与指针一样
            perror( "tp_error" );
    }
    threadpool_destroy(tp);
    close( server_sock );

    return(0);
}
