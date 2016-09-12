#include "head.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


static char host_name[HOST_NAME_LEN];

char *qq_get_host_name(void)
{
	int i;
	int fd;
	
	if (strlen(host_name) != 0)
		return host_name;

	fd = open("/etc/hostname", O_RDONLY);
	if (fd < 0)
		return NULL;

	i = 0;
	while (read(fd, &host_name[i], 1) == 1) {
		if (host_name[i] == '\n') {
			host_name[i] = 0;
			break;
		}
		i++;
	}

	close(fd);

	return host_name;
}

