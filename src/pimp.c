#define _GNU_SOURCE
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>

#define UNIX_PATH_MAX 108

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pimp.h>

#ifndef UNIX_ADDRSTRLEN
#define UNIX_ADDRSTRLEN 108
#endif /* UNIX_ADDRSTRLEN */

enum addr_type {
	ADDR_INET	= AF_INET,
	ADDR_INET6	= AF_INET6,
	ADDR_UNIX	= AF_UNIX,
};

struct addr {
	enum addr_type type;
	union {
		struct sockaddr addr;
		struct sockaddr_in in;
		struct sockaddr_in6 in6;
		struct sockaddr_un un;
	};
	socklen_t len;
};

static int unix_addr_from_str(struct addr *addr, const char *str)
{
	addr->type = ADDR_UNIX;
	addr->un.sun_family = AF_UNIX;
	snprintf(addr->un.sun_path, UNIX_PATH_MAX, "%s", str);
	if (addr->un.sun_path[0] == '@') /* address is abstract */
		addr->un.sun_path[0] = 0;

	return 0;
}

static int inet_addr_from_str(struct addr *addr, const char *str)
{
	int ret;
	char *endptr;
	char *port;
	char *ipstr;
	unsigned long ulport;
	size_t len;

	addr->type = ADDR_INET;
	addr->in.sin_family = AF_INET;
	len = strlen(str) + 1;
	ipstr = alloca(len);
	snprintf(ipstr, len, "%s", str);
	port = strchr(ipstr, ':');
	if (port == NULL)
		return -EINVAL;
	*(port++) = '\0';
	ulport = strtoul(port, &endptr, 0);
	if (*endptr != '\0' || ulport > 0xffff)
		return -EINVAL;

	ret = inet_pton(AF_INET, ipstr, &addr->in.sin_addr);
	if (ret <= 0)
		return -EINVAL;
	addr->in.sin_port = htons(ulport);

	return 0;
}

static int inet6_addr_from_str(struct addr *addr, const char *str)
{
	int ret;
	char *endptr;
	char *port;
	char *ipstr;
	unsigned long ulport;
	size_t len;

	addr->type = ADDR_INET6;
	addr->in6.sin6_family = AF_INET6;
	len = strlen(str) + 1;
	ipstr = alloca(len);
	snprintf(ipstr, len, "%s", str);
	port = strrchr(ipstr, ':');
	if (port == NULL)
		return -EINVAL;
	*(port++) = '\0';
	ulport = strtoul(port, &endptr, 0);
	if (*endptr != '\0' || ulport > 0xffff)
		return -EINVAL;

	ret = inet_pton(AF_INET6, ipstr, &addr->in6.sin6_addr);
	if (ret <= 0)
		return -EINVAL;
	addr->in6.sin6_port = htons(ulport);

	return 0;
}

static int addr_from_str(struct addr *addr, const char *str)
{
	if (addr == NULL)
		return -EINVAL;
	memset(addr, 0, sizeof(*addr));

	if (str == NULL || *str == '\0')
		return -EINVAL;

	if (strncmp(str, "unix:", 5) == 0)
		return unix_addr_from_str(addr, str + 5);
	if (strncmp(str, "inet:", 5) == 0)
		return inet_addr_from_str(addr, str + 5);
	if (strncmp(str, "inet6:", 5) == 0)
		return inet6_addr_from_str(addr, str + 6);

	return -EAFNOSUPPORT;
}

static socklen_t addr_len(struct addr *addr)
{
	if (addr == NULL)
		return 0;

	if (addr->len == 0) {
		switch (addr->type) {
		case ADDR_INET:
			addr->len = sizeof(addr->in);
			break;

		case ADDR_INET6:
			addr->len = sizeof(addr->in6);
			break;

		case ADDR_UNIX:
			addr->len = sizeof(addr->un);
			break;
		}
	}

	return addr->len;
}


FILE *pimp_server(const char *address, int flags, int backlog)
{
	int ret;
	int fd;
	int old_errno;
	FILE *server;
	struct addr addr;

	ret = addr_from_str(&addr, address);
	if (ret != 0) {
		errno = -ret;
		return NULL;
	}

	fd = socket(addr.addr.sa_family, SOCK_STREAM | flags, 0);
	if (fd == -1)
		return NULL;

	socklen_t len = addr_len(&addr);
	ret = bind(fd, &addr.addr, len);
	if (ret == -1) {
		old_errno = errno;
		close(fd);
		errno = old_errno;
		return NULL;
	}

	ret = listen(fd, backlog);
	if (ret == -1) {
		old_errno = errno;
		close(fd);
		errno = old_errno;
		return NULL;
	}
	server = fdopen(fd, "r+");
	if (server == NULL) {
		old_errno = errno;
		close(fd);
		errno = old_errno;
		return NULL;
	}
	ret = setvbuf(server, NULL, _IONBF, 0);
	if (ret != 0) {
		old_errno = errno;
		fclose(server);
		errno = old_errno;
		return NULL;
	}

	return server;
}

FILE *pimp_accept(FILE *server, int flags)
{
	int ret;
	int fd;
	int fdc;
	int old_errno;
	FILE *client;
	struct addr addr;

	if (server == NULL) {
		errno = EINVAL;
		return NULL;
	}

	fd = fileno(server);
	if (fd == -1)
		return NULL;

	addr.len = sizeof(addr.un); // TODO check that
	fdc = accept4(fd, &addr.addr, &addr.len, flags);
	if (fdc == -1)
		return NULL;
	addr.type = addr.addr.sa_family;

	client = fdopen(fdc, "r+");
	if (client == NULL) {
		old_errno = errno;
		close(fdc);
		errno = old_errno;
		return NULL;
	}
	ret = setvbuf(client, NULL, _IONBF, 0);
	if (ret != 0) {
		old_errno = errno;
		fclose(client);
		errno = old_errno;
		return NULL;
	}

	return client;
}

FILE *pimp_client(const char *address, int flags)
{
	int ret;
	int fd;
	int old_errno;
	FILE *client;
	struct addr addr;

	ret = addr_from_str(&addr, address);
	if (ret != 0) {
		errno = -ret;
		return NULL;
	}

	fd = socket(addr.addr.sa_family, SOCK_STREAM | flags, 0);
	if (fd == -1)
		return NULL;

	ret = connect(fd, &addr.addr, addr_len(&addr));
	if (ret == -1) {
		old_errno = errno;
		close(fd);
		errno = old_errno;
		return NULL;
	}

	client = fdopen(fd, "r+");
	if (client == NULL) {
		old_errno = errno;
		close(fd);
		errno = old_errno;
		return NULL;
	}
	ret = setvbuf(client, NULL, _IONBF, 0);
	if (ret != 0) {
		old_errno = errno;
		fclose(client);
		errno = old_errno;
		return NULL;
	}

	return client;
}
