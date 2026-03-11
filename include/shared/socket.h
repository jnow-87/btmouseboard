#ifndef BE_SOCKET_H
#define BE_SOCKET_H


#include <netinet/in.h>
#include <sys/socket.h>


/* types */
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

typedef struct{
	int fd;
	sockaddr_in addr;
} socket_t;


/* prototypes */
int sock_init(socket_t *sock, char const *host, unsigned int port);
void sock_close(socket_t *sock);

int sock_bind(socket_t *sock, int max_pend_clients);
int sock_connect(socket_t *sock);
int sock_accept(socket_t *sock, socket_t *client);


int sock_send(socket_t *sock, void *data, size_t n);
int sock_recv(socket_t *sock, void *data, size_t n);


#endif // BE_SOCKET_H
