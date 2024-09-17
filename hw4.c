#define  _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#define exit(N) {fflush(stdout); fflush(stderr); _exit(N); }

#define RMAX 4096

#define HMAX 1024
#define BMAX 1024
#define EMAX 16
#define PMAX 1024

static int epfd = -1;
static int pool[PMAX];

static ssize_t RSIZE = 0;
static char request[RMAX];


static int HSIZE = 0;
static char header[HMAX];

static int BSIZE = 0;
static char body[BMAX];

static const char ping_request[] = "GET /ping HTTP/1.1\r\n\r\n";
static const char ping_body[] = "pong";
static const char OK200[] = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n";

static const char echo_request[] = "GET /echo HTTP/1.1\r\n";

static int BUFSIZE = 7;
static char buffer[BMAX] = "<empty>";
static const char write_request[] = "POST /write HTTP/1.1\r\n";
static const char read_request[] = "GET /read HTTP/1.1\r\n";

static const char BR400[] = "HTTP/1.1 400 Bad Request\r\n\r\n";
static const char NF404[] = "HTTP/1.1 404 Not Found\r\n\r\n";

static const char TL413[] = "HTTP/1.1 413 Request Entity Too Large\r\n\r\n";

static int NREQUESTS = 0;
static int NHEADERS = 0;
static int NBODYS = 0;
static int NERRORS = 0;
static int NERROR_BYTES = 0;

static const char stat_request[] = "GET /stats HTTP/1.1\r\n\r\n";
static const char stat_body[] = "Requests: %d\nHeader bytes: %d\nBody bytes: %d\nErrors: %d\nError bytes: %d";

static int get_port(void);

static void send_data(int clientfd, char buf[], int size) {
	ssize_t amt, total = 0;
	do {
		amt = send(clientfd, buf + total, size - total, 0);
		total += amt;
	} while (total < size);
}

static void send_error(int clientfd, const char error[]) {
	HSIZE = strlen(error);
	memcpy(header, error, HSIZE);
	send_data(clientfd, header, HSIZE);
	close(clientfd);

	NERRORS += 1;
	NERROR_BYTES += HSIZE;
}

static void send_response(int clientfd) {
	send_data(clientfd, header, HSIZE);
	send_data(clientfd, body, BSIZE);

	NHEADERS += HSIZE;
	NBODYS += BSIZE;
	NREQUESTS += 1;
}

static void send_chunk(int clientfd) {
	int filedes = pool[clientfd];
	static int CSIZE = 0;
	static char chunk[BMAX];
	CSIZE = read(filedes, chunk, BMAX);
	if (CSIZE == 0) {
		epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, NULL);
		close(clientfd);
		close(filedes);
	} else {
		send_data(clientfd, chunk, CSIZE);
		NBODYS += CSIZE;
	}
}

static void handle_file(int clientfd) {
	char * filename = strtok(request, " \r\n");
	filename = strtok(NULL, " \r\n");
	assert(filename != NULL);
	assert(filename[0] == '/');
	filename += 1;
	int fd = open(filename, O_RDONLY, 0);
	if (fd < 0) {
		send_error(clientfd, NF404);
		return;
	}
	struct stat buf;
	if (fstat(fd, &buf) < 0) {
		perror("fstat");
		exit(1);
	}
	if (!S_ISREG(buf.st_mode)) {
		send_error(clientfd, NF404);
		return;
	}
	int fsize = buf.st_size;
	HSIZE = snprintf(header, HMAX, OK200, fsize);
	send_data(clientfd, header, HSIZE);
	NHEADERS += HSIZE;
	NREQUESTS += 1;
	static struct epoll_event Event;
	memset(&Event, 0x00, sizeof(Event));
	Event.events = EPOLLOUT;
	Event.data.fd = clientfd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &Event);
	pool[clientfd] = fd;
}

static void handle_read(int clientfd) {
	HSIZE = snprintf(header, HMAX, OK200, BUFSIZE);
	BSIZE = BUFSIZE;
	memcpy(body, buffer, BUFSIZE);
	send_response(clientfd);
	close(clientfd);
}

static void handle_write(int clientfd) {
	char * body_start = strstr(request, "\r\n\r\n");
	if (body_start == NULL) {
		send_error(clientfd, BR400);
		return;
	}
	body_start += 4;
	char * start = strstr(request, "Content-Length: ");
	start += 16;
	char * end = strstr(start, "\r\n");
	*end = '\0';
	int size = atoi(start);
	if (size > BMAX) {
		send_error(clientfd, TL413);
		return;
	}
	BUFSIZE = size;
	memcpy(buffer, body_start, BUFSIZE);
	handle_read(clientfd);

}

static void handle_echo (int clientfd) {
	char * start = strstr(request, "\r\n");
	start += 2;
	char * end = strstr(start, "\r\n\r\n");
	if (end == NULL) {
		send_error(clientfd, BR400);
		return;
	}
	*end = '\0';
	BSIZE = strlen(start);
	if (BSIZE > BMAX) {
		send_error(clientfd, TL413);
		return;
	}
	memcpy(body, start, BSIZE);
	HSIZE = snprintf(header, HMAX, OK200, BSIZE);
	send_response(clientfd);
	close(clientfd);
}

static void handle_ping(int clientfd) {
	HSIZE = snprintf(header, HMAX, OK200, 4);
	BSIZE  = strlen(ping_body);
	memcpy(body, ping_body, BSIZE);
	send_response(clientfd);
	close(clientfd);
}

static void handle_stat(int clientfd) {
	BSIZE = snprintf(body, BMAX, stat_body, NREQUESTS, NHEADERS, NBODYS, NERRORS, NERROR_BYTES);
	HSIZE = snprintf(header, HMAX, OK200, BSIZE);
	send_response(clientfd);
	close(clientfd);
}

static void handle_request(int clientfd) {
	RSIZE = recv(clientfd, request, RMAX, 0);
	request[RSIZE] = '\0';

	if (RSIZE == 0) {
		close(clientfd);
		return;
	}
	
	if (!strncmp(request, ping_request, strlen(ping_request))) {
		handle_ping(clientfd);
	} else if (!strncmp(request, echo_request, strlen(echo_request))) {
		handle_echo(clientfd);
	} else if (!strncmp(request, write_request, strlen(write_request))) {
                handle_write(clientfd);
        } else if (!strncmp(request, read_request, strlen(read_request))) {
                handle_read(clientfd);
        } else if (!strncmp(request, stat_request, strlen(stat_request))) {
		handle_stat(clientfd);
	} else if (!strncmp(request, "GET ", 4)) {
		handle_file(clientfd);
	} else {
		send_error(clientfd, BR400);
	}
	

}

static int open_listenfd(int port) {
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	static struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

	//allows for reuse
	int optval = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	bind(listenfd, (struct sockaddr*)&server, sizeof(server));

	listen(listenfd, 10);
	return listenfd;
}

static int accept_client(int listenfd) {
	static struct sockaddr_in client;
	static socklen_t csize = 0;
	memset(&client, 0x00, sizeof(client));
	int clientfd = accept(listenfd, (struct sockaddr*)&client, &csize);
	return clientfd;
}

static void event_loop(int listenfd) {
	epfd = epoll_create1(0);
	struct epoll_event Event;
	memset(&Event, 0x00, sizeof(Event));
	Event.events = EPOLLIN;
	Event.data.fd = listenfd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &Event);
	struct epoll_event Events[EMAX];
	memset(&Events, 0x00, sizeof(Events));
	while (1) {
		int nfds = epoll_wait(epfd, Events, EMAX, -1);
		for (int i = 0; i < nfds; ++i) {
			int fd = Events[i].data.fd;
			if (Events[i].events == EPOLLIN) {
				if (fd == listenfd) {
					int clientfd = accept_client(listenfd);
					memset(&Event, 0x00, sizeof(Event));
					Event.events = EPOLLIN;
					Event.data.fd = clientfd;
					epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &Event);
				} else {
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					handle_request(fd);
				}
			} else {
				send_chunk(fd);
			}
		}
	}
}

int main(int argc, char * argv[])
{
    int port = get_port();

    printf("Using port %d\n", port);
    printf("PID: %d\n", getpid());

    // Make server available on port
    int listenfd = open_listenfd(port);

    // Process client requests
    event_loop(listenfd);

    return 0;
}


static int get_port(void)
{
    int fd = open("port.txt", O_RDONLY);
    if (fd < 0) {
        perror("Could not open port.txt");
        exit(1);
    }

    char buffer[32];
    int r = read(fd, buffer, sizeof(buffer));
    if (r < 0) {
        perror("Could not read port.txt");
        exit(1);
    }

    return atoi(buffer);
}

