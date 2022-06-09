#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
int main(int argc, char *argv[])
{
    int server_socket_fd;
    int client_socket_fd;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(54154); //alternative port to 80
    inet_aton("172.22.174.232", &server_address.sin_addr);
    memset(&server_address.sin_zero, '\0', sizeof(server_address.sin_zero));
    bind(server_socket_fd, (struct sockaddr *) &server_address, sizeof(server_address));
    listen(server_socket_fd, 10);
    while (1) 
    {
        printf("Mhmm listening for the request..\n");
        int c_len = sizeof(client_address);
        client_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client_address, &c_len);
        int rv = 0;
        char buf[5000];
        while(rv = read(client_socket_fd, buf, rv))
        {
            write(1, buf, rv);
        }
        write(client_socket_fd, "hello this is the test complete", 32);
        printf("Bye bye the test has been completed\n");
        close(client_socket_fd);
    }
    close(server_socket_fd);
    return 0;
}