/**
 * Author:  xiepingfu
 * Data:    2019.10.31
 * Comment: main funciton, client, server
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "parser.h"
#include "utils.h"

#define MAXLINE 2048
#define SERV_PORT 9102
#define MAX_BACK_LOG 20

const char server_welcome[] = "\
Welcome to xpfsql\n\
Version: v1.0\n\
xpfsql server start.\n";

const char header[] = "xpfsql> ", muiltline[] = "......> ";
const char client_welcome[] = "\
Welcome to xpfsql\n\
Version: v1.0\n\
xpfsql client start.\n";

int start_server()
{
    write(STDOUT_FILENO, server_welcome, strlen(server_welcome));
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddr_len;
    int listenfd, connfd;
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN];
    char msg[MAXLINE];
    int i, n, pid;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    listen(listenfd, MAX_BACK_LOG);

    while (1)
    {
        cliaddr_len = sizeof(cliaddr);
        connfd = accept(listenfd,
                        (struct sockaddr *)&cliaddr, &cliaddr_len);

        pid = fork();
        if (pid == -1)
        {
            write(STDOUT_FILENO, "Fork error!\n", strlen("Fork error!\n"));
            return 1;
        }
        else if (pid == 0)
        {
            sprintf(msg, "Connect %d.\n", pid);
            write(STDOUT_FILENO, msg, strlen(msg));
            fflush(stdout);
            XBParser xbParser;
            xbParser.db = NULL;
            xbParser.clientfd = connfd;
            close(listenfd);
            while (1)
            {
                memset(buf, '\0', sizeof(buf));
                n = read(connfd, buf, MAXLINE);
                xbParser.sql = buf;
                sprintf(msg, "rec: %s\n", buf);
                write(STDOUT_FILENO, msg, strlen(msg));
                if (strcmp(".quit", buf) == 0)
                {
                    write(STDOUT_FILENO, "Closed a connection.\n", strlen("Closed a connection.\n"));
                    close(connfd);
                    return 0;
                }
                ParseSQL(&xbParser);
                write(connfd, "\n", strlen("\n"));
                write(connfd, ".end", strlen(".end"));
            }
            close(connfd);
            exit(0);
        }
    }
    return 0;
}

int preprocess(char *buf, int sz)
{
    for (int i = 0; buf[i] != '\0' && i < sz; ++i)
    {
        if (buf[i] == ';')
            return i;
    }
    return sz;
}

int start_client(char *ip_address, char *port)
{
    write(STDOUT_FILENO, client_welcome, strlen(client_welcome));
    struct sockaddr_in servaddr;
    char sql[MAXLINE], buf[MAXLINE], rec[MAXLINE], msg[MAXLINE];
    int sockfd, str_size, end_size;
    size_t sql_size = 0;
    int rc = 0;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_address, &servaddr.sin_addr);
    int serv_port;
    if (StringToInt(port, &serv_port) != 0)
    {
        write(STDOUT_FILENO, "Port error!\n", strlen("Port error!\n"));
        return -1;
    }
    servaddr.sin_port = htons(serv_port);
    if ((rc = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
    {
        sprintf(msg, "Connected %s:%s fail.\n", ip_address, port);
        write(STDOUT_FILENO, msg, strlen(msg));
        return rc;
    }
    sprintf(msg, "Connected %s:%s\n", ip_address, port);
    write(STDOUT_FILENO, msg, strlen(msg));
    write(STDOUT_FILENO, header, strlen(header));
    while (1)
    {
        str_size = read(STDIN_FILENO, buf, MAXLINE);
        if (sql_size == 0 && str_size > 0 && buf[0] == '.')
        {
            if (strcmp(buf, ".quit"))
            {
                write(sockfd, ".quit", strlen(".quit"));
                write(STDOUT_FILENO, "Bye!\n", strlen("Bye!\n"));
                fflush(stdout);
                break;
            }
        }
        end_size = preprocess(buf, str_size);

        for (int i = 0; i <= end_size && i < str_size; ++i, sql_size++)
        {
            sql[sql_size] = buf[i];
        }
        if (end_size == str_size)
        {
            write(STDOUT_FILENO, muiltline, strlen(muiltline));
            fflush(stdout);
        }
        else
        {
            write(sockfd, sql, sql_size);
            while (read(sockfd, rec, MAXLINE) != -1)
            {
                int l = strlen(rec);
                if (l >= 4)
                {
                    if (rec[l - 1] == 'd' && rec[l - 2] == 'n' && rec[l - 3] == 'e' && rec[l - 4] == '.')
                    {
                        rec[l - 4] = '\0';
                        write(STDOUT_FILENO, rec, strlen(rec));
                        bzero(rec, sizeof(rec));
                        break;
                    }
                }
                write(STDOUT_FILENO, rec, strlen(rec));
                bzero(rec, sizeof(rec));
                fflush(stdout);
            }
            // write(STDOUT_FILENO, rec, strlen(rec));
            sql_size = 0;
            memset(buf, 0, sizeof(buf));
            write(STDOUT_FILENO, header, strlen(header));
        }
    }
    close(sockfd);
    return 0;
}

int main(int argc, char *argv[])
{
    char ch;
    char port[10], ip_address[20];
    int is_server = 1;

    while ((ch = getopt(argc, argv, "p:h:")) != EOF)
    {
        switch (ch)
        {
        case 'p':
        {
            is_server = 0;
            strcpy(port, optarg);
            // printf("P:%s\n", optarg);
            break;
        }
        case 'h':
        {
            is_server = 0;
            strcpy(ip_address, optarg);
            // printf("H:%s\n", optarg);
            break;
        }
        case '?':
        default:
        {
            return 1;
        }
        }
    }
    if (is_server)
    {
        return start_server();
    }
    else
    {
        return start_client(&ip_address[0], &port[0]);
    }
}