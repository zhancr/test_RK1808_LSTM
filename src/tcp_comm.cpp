#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <unistd.h> 
#include <arpa/inet.h> 

#include "tcp_comm.h"

#define SERVER_PORT	1808
#define MAXDATASIZE	100  
#define SERVER_IP	"192.168.180.8" 


tcp_comm::tcp_comm()
{
	client_sockfd = -1;
	listen_sockfd = -1;

	tcpc_saddr.sin_family = AF_INET; 
	tcpc_saddr.sin_port = htons(SERVER_PORT); 
	tcpc_saddr.sin_addr.s_addr = inet_addr(SERVER_IP); 

	tcps_saddr.sin_family = AF_INET; 
	tcps_saddr.sin_port = htons(SERVER_PORT); 
	tcps_saddr.sin_addr.s_addr = htonl(INADDR_ANY); 
}

int tcp_comm::tcpc_connect()
{
	if ((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { 
		perror("socket init failed.\n"); 
		return -1; 
	}

	printf("tcp client client_sockfd: %d\n", client_sockfd);

	if (connect(client_sockfd, (struct sockaddr *)&tcpc_saddr, sizeof(struct sockaddr_in)) == -1) {
		perror("connect server error\n"); 
		return -1;
	} 

	printf("\n============connect server succeed=============\n"); 

	return 0;
}

int tcp_comm::tcps_accept()
{
	int on;
	socklen_t client_len;

	if ((listen_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { 
		perror("socket init failed.\n"); 
		return -1; 
	}

	printf("tcp server listen_sockfd: %d\n", listen_sockfd);

	setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	/*bind a socket or rename a sockt*/
	if (bind(listen_sockfd, (struct sockaddr*)&tcps_saddr, sizeof(struct sockaddr_in)) == -1) {
		printf("bind error\n");
		return -1;
	}

	if(listen(listen_sockfd, 1024) == -1) {
		printf("listen error\n");
		return -1;
	}

	client_len = sizeof(client_saddr);

	while (1) {
		if((client_sockfd = accept(listen_sockfd, (struct sockaddr*)&client_saddr,
					   &client_len)) == -1) {
			printf("accept error\n");
			return -1;
		} else {
			printf("create connection successfully\n");
			printf("tcp server client_sockfd: %d\n", client_sockfd);
			break;
		}
	}

	return 0;
}

ssize_t tcp_comm::send_data(const void *buf, size_t len)
{
	ssize_t send_bytes = 0;
	size_t send_len = 0;

	while (send_len < len) {
		if ((send_bytes = send(client_sockfd, (char *)buf + send_len,
					len - send_len, 0)) == -1) {
			perror("send error\n");
			goto exit;
		}
		send_len += send_bytes;
	}

exit:
	return send_len;
}

ssize_t tcp_comm::recv_data(void *buf, size_t len)
{
	ssize_t recv_bytes = 0;
	size_t recv_len = 0;

	while (recv_len < len) {
		if ((recv_bytes = recv(client_sockfd, (char *)buf + recv_len,
					len - recv_len, 0)) == -1) {
			perror("recv error\n");
			goto exit;
		}
		recv_len += recv_bytes;
	}

exit:
	return recv_len;
}

void tcp_comm::destroy(void)
{
	if (listen_sockfd >= 0)
		close(listen_sockfd);
	if (client_sockfd >= 0)
		close(client_sockfd);
}

tcp_comm::~tcp_comm()
{

}

