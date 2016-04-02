#include "chan.h"
#include "chan_def.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// TODO
// Mutex real implementation
// Better channel handling (remove closed etc)

struct chan* chan_create(void)
{
	struct chan* c = (struct chan*) malloc(sizeof(struct chan));
	
	if (c == NULL)
	{
		return NULL;
	}

	if (pipe(c->fds))
	{
		perror("chan_create::pipe");
		free(c);
		return NULL;
		
	}
		
        if (fcntl(c->fds[READ_FD], F_SETFL, O_NONBLOCK) == -1)
	{
		perror("chan_create::fncntl");
		free(c);
		return NULL;	
	}

#ifdef MT_SAFE
        c->l = lk_create();
#endif

	return c;
}

void chan_close(struct chan* c)
{
	if (c == NULL)
	{
		return;
	}

	close(c->fds[WRITE_FD]);
	c->fds[WRITE_FD] = -1;
}

void chan_destroy(struct chan* c)
{
	if (c == NULL)
	{
		return;
	}

	if (c->fds[WRITE_FD] > -1)
	{
		chan_close(c);
	}
	close(c->fds[READ_FD]);

#ifdef MT_SAFE
        lk_destroy(c->l);
        c->l = NULL;
#endif

	free(c);
}

int chan_write(struct chan* c, struct chan_msg* m)
{
	ssize_t bw;

	bw = write(c->fds[WRITE_FD], m, sizeof(struct chan_msg));
	if (bw != sizeof(struct chan_msg))
	{
		return -1;
	}

	return 0;
}

int read_msg(struct chan* c, struct chan_msg* m)
{
	int result;
	ssize_t br;

#ifdef MT_SAFE
        if (lk_lock(c->l))
	{
		perror("chan_read::lock");
		// FIXME
		return -1;
	}
#endif

	br = read(c->fds[READ_FD], m, sizeof(struct chan_msg));
#ifdef MT_SAFE
        lk_unlock(c->l);
#endif
	if (br == 0)
	{
		result = EBADF;
	}
	else if (br == sizeof(struct chan_msg))
	{
		result = 0;
	}
	else 
	{
		if(errno == EBADF)
		{
			result = EBADF;
		}
		else if (errno == EAGAIN)
		{
			result = EAGAIN;
		}
		else 
		{
			printf("read_msg:Read error: %ld\n", br);
			result = -1;			
		}
	}
	
	return result;
}
