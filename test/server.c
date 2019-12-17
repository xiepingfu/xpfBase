#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "parser.h"

#define MAXLINE 1024
#define SERV_PORT 9102
#define MAX_BACK_LOG 20

const char server_welcome[] = "Welcome to xpfsql\n\
Version: v1.0\n\
xpfsql server start.\n";

void output_server_welcome()
{
	write(STDOUT_FILENO, server_welcome, strlen(server_welcome));
}

int start_server()
{
	output_server_welcome();
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddr_len;
	int listenfd, connfd;
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN];
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
			perror("Fork error!\n");
			return 1;
		}
		else if (pid == 0)
		{
			XBParser xbParser;
			close(listenfd);
			while (1)
			{
				memset(buf, '\0', sizeof(buf));
				n = read(connfd, buf, MAXLINE);
				xbParser.sql = buf;
				write(STDOUT_FILENO, buf, n);
				if (StringCompare(".quit", 5, "buf", n) == 0)
				{
					write(connfd, buf, n);
					printf("Closed a connection.\n");
					close(connfd);
					return 0;
				}
				if (ParseSQL(&xbParser) == RC_OK)
				{
					printf("ParseSQL(buf)\n");
				}
			}
			close(connfd);
		}
		else
		{
			close(connfd);
		}
	}
	return 0;
}