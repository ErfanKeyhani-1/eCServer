#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFSIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFSIZE];
    char resp_header[128];

    // 1) Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // 2) Allow quick reuse of the port
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    // 3) Bind to 0.0.0.0:4221
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(4221);

    if (bind(server_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    // 4) Listen
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port 4221...\n");

    // 5) Accept loop
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        // 6) Read the request (up to end of headers)
        ssize_t total = 0, n;
        memset(buffer, 0, BUFSIZE);
        while ((n = read(client_fd, buffer + total, BUFSIZE - 1 - total)) > 0) {
            total += n;
            buffer[total] = '\0';
            if (strstr(buffer, "\r\n\r\n")) break;
        }
        if (n < 0) {
            perror("read");
            close(client_fd);
            continue;
        }

        // Parse method and path
        char method[16], path[256];
        if (sscanf(buffer, "%15s %255s", method, path) != 2) {
            close(client_fd);
            continue;
        }

        // 8) Handle GET /
        if (strcmp(method, "GET") == 0 && strcmp(path, "/") == 0) {
            const char* body = "Hello! This is the root.\n";
            size_t len = strlen(body);
            int hlen = snprintf(resp_header, sizeof(resp_header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %zu\r\n"
                "\r\n",
                len);
            send(client_fd, resp_header, hlen, 0);
            send(client_fd, body, len, 0);

            // Handle GET /echo/<msg>
        }
        else if (strcmp(method, "GET") == 0 && strncmp(path, "/echo/", 6) == 0) {
            const char* to_echo = path + 6;
            size_t len = strlen(to_echo);
            int hlen = snprintf(resp_header, sizeof(resp_header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %zu\r\n"
                "\r\n",
                len);
            send(client_fd, resp_header, hlen, 0);
            send(client_fd, to_echo, len, 0);
            printf("[+] User wanted to echo: %s \n", to_echo);
            //Anything else: 404
        }
        else {
            const char* body = "404 Not Found\n";
            size_t len = strlen(body);
            int hlen = snprintf(resp_header, sizeof(resp_header),
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %zu\r\n"
                "\r\n",
                len);
            send(client_fd, resp_header, hlen, 0);
            send(client_fd, body, len, 0);
        }

        close(client_fd);
    }

    // (never reached)
    close(server_fd);
    return 0;
}
s
