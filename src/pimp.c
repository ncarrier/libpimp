#include <stdlib.h>
#include <errno.h>

#include <pimp.h>

FILE *pimp_server(const char *address)
{
	errno = ENOSYS;

	return NULL;
}

FILE *pimp_accept(FILE *server)
{
	errno = ENOSYS;

	return NULL;
}

FILE *pimp_client(FILE *server)
{
	errno = ENOSYS;

	return NULL;
}
