#ifndef _PIMP_H
#define _PIMP_H
#include <stdio.h>

FILE *pimp_server(const char *address);

FILE *pimp_accept(FILE *server);

FILE *pimp_client(FILE *server);

#endif /* _PIMP_H */
