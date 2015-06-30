/* Pimp Implements Minimalistically a Printf oriented message protocol */
#ifndef _PIMP_H
#define _PIMP_H
#include <stdio.h>

#define PIMP_API __attribute__((visibility("default")))

PIMP_API FILE *pimp_server(const char *address, int flags, int backlog);

PIMP_API FILE *pimp_accept(FILE *server, int flags);

PIMP_API FILE *pimp_client(const char *address, int flags);

#endif /* _PIMP_H */
