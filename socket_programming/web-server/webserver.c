#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include<errno.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<time.h>
#include<unistd.h>
#define MAXREQ 2048

void handle_request(char *buf, int client_socket, int rv)
{
        write(1, buf, rv);
        char *l = strstr(buf, "GET");
        strtok(l, " ");
        char *required = strtok(NULL, " ");
        int fd = open(required + 1, O_RDONLY);
        if(fd > 0)
        {
            write(client_socket, "HTTP/1.1 200 OK\n", 17);
            char body[MAXREQ];
            int acrv;
            while (acrv = read(fd, body, MAXREQ))
            {
                write(client_socket, body, acrv);
            }
            close(fd);
        }
        else
            write(client_socket, "HTTP/1.1 404 NOT FOUND\n", 24);
        return;
              
}

void add_logs(int logs_fd, char *buf, int rv, struct sockaddr_in client_addr)
{
    char log_entry[MAXREQ + 300];
    time_t now;
    time(&now);
    int count = sprintf(log_entry, "%s:%s:%d\n%s", strtok(ctime(&now), "\n"), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buf);
    write(logs_fd, log_entry, count);
}

int main(int argc, char **argv)
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0); //server socket
    int client_socket; //this is client socket
    int logs_fd = open("log.txt", O_APPEND|O_RDWR); //the log file

    struct sockaddr_in server_addr;//server address
    struct sockaddr_in client_addr;//client address

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(54154);//converting the endianess 
    inet_aton(argv[1], &server_addr.sin_addr);
    memset(&server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 10);

    while (1)
    {
        printf("Waiting for the Request...\n");
        int c_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &c_len);
        printf("Request has been received..\n");
        int rv = 0;
        char buf[MAXREQ];
        rv = recv(client_socket, buf, MAXREQ, 0);
        add_logs(logs_fd, buf, rv, client_addr); //adding the logs
        handle_request(buf, client_socket, rv);//handling the request
        printf("Request has been entertained..\n");
        memset(buf, '\0', rv);
        close(client_socket);
    }
    close(server_socket);
    close(logs_fd);
    return 0;
}