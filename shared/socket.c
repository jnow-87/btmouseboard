#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <shared/socket.h>


/* global functions */
int sock_init(socket_t *sock, char const *host, unsigned int port){
	struct addrinfo *info,
					*addr;


	sock->fd = socket(AF_INET, SOCK_STREAM, 0);

	if(sock->fd == -1)
		return -1;

	memset(&sock->addr, 0, sizeof(sock->addr));

	sock->addr.sin_family = AF_INET;
	sock->addr.sin_port = htons(port);
	sock->addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(host == 0x0)
		return 0;

	if(getaddrinfo(host, 0x0, 0x0, &info) != 0)
		return -1;

	for(addr=info; addr!=0x0; addr=addr->ai_next){
		sock->addr.sin_addr = ((struct sockaddr_in*)addr->ai_addr)->sin_addr;

		if(connect(sock->fd, (sockaddr*)&sock->addr, sizeof(sock->addr)) == 0)
			break;
	}

	freeaddrinfo(info);

	if(addr == 0x0)
		sock_close(sock);

	return -(addr == 0x0);
}

void sock_close(socket_t *sock){
	close(sock->fd);
	sock->fd = -1;
}

int sock_bind(socket_t *sock, int max_pend_clients){
	sock->fd = socket(AF_INET, SOCK_STREAM, 0);

	if(sock->fd == -1)
		goto err_0;

	if(bind(sock->fd, (sockaddr*)&sock->addr, sizeof(sock->addr)) < 0)
		goto err_1;

	if(listen(sock->fd, max_pend_clients) != 0)
		goto err_1;

	return 0;


err_1:
	sock_close(sock);

err_0:
	return -1;
}

int sock_connect(socket_t *sock){
	if(sock->fd != -1)
		return 0;

	sock->fd = socket(AF_INET, SOCK_STREAM, 0);

	if(sock->fd == -1)
		goto err_0;

	if(connect(sock->fd, (sockaddr*)&sock->addr, sizeof(sock->addr)) != 0)
		goto err_1;

	return 0;


err_1:
	sock_close(sock);

err_0:
	return -1;
}

int sock_accept(socket_t *sock, socket_t *client){
	socklen_t size = sizeof(client->addr);


	client->fd = accept(sock->fd, (sockaddr*)&client->addr, &size);

	if(client->fd == -1)
		return -1;

	return 0;
}

int sock_send(socket_t *sock, void *data, size_t n){
	if(send(sock->fd, data, n, 0) != n)
		return -1;

	return 0;
}

int sock_recv(socket_t *sock, void *data, size_t n){
	if(recv(sock->fd, data, n, MSG_WAITALL) != n)
		return -1;

	return 0;
}
