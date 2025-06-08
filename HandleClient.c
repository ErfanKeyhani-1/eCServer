#include "HandleClient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#define BUFSIZE 1024

void HandleClient(int client_fd) {
    char buffer[BUFSIZE];
    ssize_t total = 0, n;

    // Read until end of headers ("\r\n\r\n") or buffer fills
    memset(buffer, 0, BUFSIZE);
    while ((n = read(client_fd, buffer + total, BUFSIZE-1-total)) > 0) {
        total += n;
        buffer[total] = '\0';
        if (strstr(buffer, "\r\n\r\n")) break;
    }
    if (n < 0) {
        perror("read");
        close(client_fd);
        return;
    }

    // Parse method and path
    char method[16], path[256];
    if (sscanf(buffer, "%15s %255s", method, path) != 2) {
        // malformed request
        close(client_fd);
        return;
    }

    char resp_header[256];

    // ROUTING
    if (strcmp(method, "GET")==0 && strcmp(path, "/")==0) {
        const char *body = "Hello! This is the root.\n";
        size_t len = strlen(body);
        int hlen = snprintf(resp_header, sizeof(resp_header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %zu\r\n"
            "\r\n", len);
        send(client_fd, resp_header, hlen, 0);
        send(client_fd, body, len, 0);

    } else if (strcmp(method,"GET")==0 && strncmp(path,"/echo/",6)==0) {
        const char *to_echo = path + 6;
        size_t len = strlen(to_echo);
        int hlen = snprintf(resp_header, sizeof(resp_header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %zu\r\n"
            "\r\n", len);
        send(client_fd, resp_header, hlen, 0);
        send(client_fd, to_echo, len, 0);
        printf("[+] Echoed: %s\n", to_echo);

    } else if (strcmp(method,"GET")==0 && strcmp(path,"/User-Agent")==0) {
        char *ua = strstr(buffer, "\r\nUser-Agent:");
        if (ua) {
            ua += 2 + strlen("User-Agent:"); 
            while (*ua==' ') ua++;
            char *end = strstr(ua, "\r\n");
            if (end) {
                size_t len = end - ua;
                int hlen = snprintf(resp_header, sizeof(resp_header),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %zu\r\n"
                    "\r\n", len);
                printf("[+] User-Agent: %.*s\n",(int)len, ua);
                send(client_fd, resp_header, hlen, 0);
                send(client_fd, ua, len, 0);
            } else {
                fprintf(stderr,"Missing line end after User-Agent\n");
            }
        } else {
            fprintf(stderr,"User-Agent header not found\n");
        }

    } else {
        // 404 fallback
        const char *body = "404 Not Found\n";
        size_t len = strlen(body);
        int hlen = snprintf(resp_header, sizeof(resp_header),
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %zu\r\n"
            "\r\n", len);
        send(client_fd, resp_header, hlen, 0);
        send(client_fd, body, len, 0);
    }

    close(client_fd);
}