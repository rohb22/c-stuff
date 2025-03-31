#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>


#define N_CONN 5
#define BUFFER_SIZE 4096
#define MAX_PORT_ATTEMPTS 10

int SOCKET;

void close_sock(int sig){
    printf("\nClosing socket......\n");
    shutdown(SOCKET, SHUT_RDWR);
    close(SOCKET);
    printf("Socket closed\n");
    exit(0);
}


int main(int argc, char **argv ) {
    
    // check arguments for port number
    if (argc < 2) {
        printf("Missing Port\n");
        return -1;
    }

    // parse the port (atoi converts string to numerical)
    int PORT = atoi(argv[1]);

    // atoi returns 0 if its not a valid numerical
    if (PORT == 0) {
        printf("Numerical Port needed\n");
        return -1;
    }

    // create a socket
    SOCKET = socket(AF_INET, SOCK_STREAM, 0);


    // socket returns -1 if failed
    if(SOCKET == -1) {
        printf("Cant create a socket\n");
        return -1;  
    } else {
        printf("Socket established\n");
    }
        
    // initialize sockaddr 
    struct sockaddr_in SOCKADDR;
    SOCKADDR.sin_family = AF_INET;
    SOCKADDR.sin_addr.s_addr = htonl(INADDR_ANY);
    SOCKADDR.sin_port = htons(PORT);
 
    // this closes the socket and port when interrupted
    signal(SIGINT, close_sock);
    // this makes the port reusable again after termination
    int opt = 1;
    setsockopt(SOCKET, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    int port_attempts = 0;
    while(bind(SOCKET, (struct sockaddr *) &SOCKADDR, sizeof SOCKADDR) == -1) {
        if(port_attempts >= MAX_PORT_ATTEMPTS) {
            printf("Cant find a free port... Closing socket\n");
            close(SOCKET);
            exit(0);
        }
        printf("Cant bind to PORT:%d\n", PORT);
        PORT++;
        port_attempts++;
        printf("Attempting to bind on PORT:%d\n",PORT);
    }
    printf("Binded port: %d\n", PORT);

    printf("Attempting to listen to port: %d\n", PORT);
    if(listen(SOCKET, N_CONN) == -1){
        printf("failed to listen\n");
        return -1;
    }else {
        printf("Listening to Port: http://localhost:%d\n", PORT);
    }

    while(true) {
        int addr_size = sizeof SOCKADDR;
        // accepts socket connections
        int sock_fd = accept(SOCKET, (struct sockaddr *) &SOCKADDR, &addr_size);
        if(sock_fd == -1) {
            printf("Failed to accept\n");
            return -1;
        }

        char req[BUFFER_SIZE];
        recv(sock_fd, req, sizeof req -1, 0);

        printf("%s\n", req);
        printf("%d\n",strncmp(req,"GET /HTTP",4));
        if(strncmp(req, "GET / HTTP/1.1", 4) != 0){
            const char *response = 
                    "HTTP/1.1 405 Method Not Allowed\r\n"
                    "Content-Length: 25\r\n"
                    "Content-Type: text/plain\r\n"
                    "\r\n"
                    "Unsupported Request Method";
            
            send(sock_fd, response, strlen(response), 0);
            close(sock_fd);
        }
        // open file
        FILE *file = fopen("index.html", "r");
        if (!file) {
            const char *response =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 13\r\n"
                "Content-Type: text/plain\r\n"
                "\r\n"
                "404 Not Found";
            send(sock_fd, response, strlen(response), 0);
        } else {
            char content[BUFFER_SIZE] = {0};
            fread(content, 1, BUFFER_SIZE - 1, file);
            fclose(file);

            char response[BUFFER_SIZE];
            snprintf(response, sizeof(response),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Length: %ld\r\n"
                    "Content-Type: text/html\r\n"
                    "\r\n"
                    "%s",
                    strlen(content), content);

            send(sock_fd, response, strlen(response), 0);
        }
       
        close(sock_fd);
    }
    return 0;
}