#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <unistd.h>

#define UNIX_PATH_MAX 108

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pimp.h>

FILE *pimp_server(const char *address, int flags, int backlog)
{
	int ret;
	int fd;
	int old_errno;
	FILE *server;
	struct sockaddr_un addr;

	if (address == NULL || *address == '\0') {
		errno = EINVAL;
		return NULL;
	}

	if (strncmp(address, "unix:", 5) != 0) {
		errno = EINVAL;
		return NULL;
	}

	fd = socket(AF_UNIX, SOCK_STREAM | flags, 0);
	if (fd == -1)
		return NULL;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, UNIX_PATH_MAX, "%s", address + 5);
	if (addr.sun_path[0] == '@') /* address is abstract */
		addr.sun_path[0] = 0;
	ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
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
	struct sockaddr_un addr;
	socklen_t addrlen;

	if (server == NULL) {
		errno = EINVAL;
		return NULL;
	}

	fd = fileno(server);
	if (fd == -1)
		return NULL;

	fdc = accept4(fd, (struct sockaddr *)&addr, &addrlen, flags);
	if (fdc == -1)
		return NULL;

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
	struct sockaddr_un addr;

	if (address == NULL || *address == '\0') {
		errno = EINVAL;
		return NULL;
	}

	if (strncmp(address, "unix:", 5) != 0) {
		errno = EINVAL;
		return NULL;
	}

	fd = socket(AF_UNIX, SOCK_STREAM | flags, 0);
	if (fd == -1)
		return NULL;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, UNIX_PATH_MAX, "%s", address + 5);
	if (addr.sun_path[0] == '@') /* address is abstract */
		addr.sun_path[0] = 0;
	ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
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
