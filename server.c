#include "HandleClient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT    4221
#define BACKLOG 5

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return EXIT_FAILURE;
    }

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd,
                           (struct sockaddr*)&client_addr,
                           &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        pid_t pid = fork();
        if(pid < 0) {
            perror("[-] Uhm excuse me what in actual fuck is happening to fork()?, well it failed. \n");
            close(client_fd);
            continue;
        } else if(pid == 0){
            /*
            +++++++++++++++++IN THE CHILD++++++++++++++++++
            */
            close(server_fd);
            printf("[+] You are now in the child process");
            HandleClient(client_fd);

        } else {
            perror("[-] dont know wtf just happened");
        }
    }

    // unreachable here
    close(server_fd);
    return EXIT_SUCCESS;
}
