#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <error.h>
#include <errno.h>

#include <pimp.h>

static void str_free(char **str)
{
	if (str == NULL || *str == NULL)
		return;

	free(*str);
	*str = NULL;
}

int main(int argc, char *argv[])
{
	char __attribute__((cleanup(str_free)))*buf = NULL;
	size_t len = 0;
	bool is_server;
	const char *mode;
	const char *address;
	FILE *server;
	FILE *client;

	if (argc != 3)
		error(EXIT_FAILURE, 0, "usage %s client|server address\n",
				basename(argv[0]));
	mode = argv[1];
	address = argv[2];

	is_server = strcmp(mode, "server") == 0;
	if (!is_server && strcmp(mode, "client") != 0)
		error(EXIT_FAILURE, 0, "first argument must be \"server\" or "
				"\"client\"\n");
	if (is_server) {
		server = pimp_server(address, 0, 10);
		if (server == NULL)
			error(EXIT_FAILURE, errno, "pimp_server");

		client = pimp_accept(server, 0);
		if (client == NULL)
			error(EXIT_FAILURE, errno, "pimp_accept");

		while ((getline(&buf, &len, client) != -1))
			printf("received: \"%.*s\"\n", (int)(strlen(buf) - 1),
					buf);
	} else {
		client = pimp_client(address, 0);
		if (client == NULL)
			error(EXIT_FAILURE, errno, "pimp_client");

		while ((getline(&buf, &len, stdin) != -1))
			fprintf(client, "*** \"%.*s\" ***\n",
					(int)(strlen(buf) - 1), buf);
	}

	return EXIT_SUCCESS;
}
