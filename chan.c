#include "chan.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#ifdef USE_SELECT
# include <string.h>
# include <sys/time.h>
#else
# include <poll.h>
#endif
#include <fcntl.h>
#include <errno.h>
#ifdef MT
# include <pthread.h>
#endif

// TODO
// Mutex real implementation
// Better channel handling (remove closed etc)

#define READ_FD 0
#define WRITE_FD 1

struct chan
{
	int fds[2];
#ifdef MT
	pthread_mutex_t rlock;
#endif
};

/**
 * Read one msg from fd.
 * @param the channel to read from
 * @param the message to read into.
 * @return 0 on sucess.
 *         EAGAIN if timeout.
 *         EBADF if fd is closed.
 *         -1 if error occured.
 */
static int read_msg(struct chan*, struct chan_msg*);

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

#ifdef MT
        pthread_mutex_init(&c->rlock, NULL);
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
	free(c);
}

#ifdef USE_SELECT
int chan_read(struct chan* c, struct chan_msg* m, int timeout)
{
	fd_set fds;
	struct timeval tv;
	struct timeval* ptv;
	int ready;
	int result = EAGAIN;

	// Timeout is in milliseconds
	if (timeout < 0)
	{
		ptv = NULL;
	}
	else 
	{
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout - tv.tv_sec * 1000) * 1000;
		ptv = &tv;
	}

	FD_ZERO(&fds);
	FD_SET(c->fds[READ_FD], &fds);
	ready = select(c->fds[READ_FD] + 1, 
		       &fds, // Read fds
		       NULL,   // write fds
		       NULL,   // error fds
		       ptv);
	if (ready > 0)
	{
		ssize_t br;

		if (FD_ISSET(c->fds[READ_FD], &fds))
		{
#ifdef MT
                        if (pthread_mutex_lock(&c->rlock))
			{
				perror("chan_read::pthread_mutex_lock");
				// FIXME
				return -1;
			}
#endif

                        br = read(c->fds[READ_FD], m, sizeof(struct chan_msg));
#ifdef MT
                        pthread_mutex_unlock(&c->rlock);
#endif
			if (br == 0)
			{
				// The pipe is empty AND closed.
				// Strange though that POLLIN was set...
				result = EBADF;
//				break;
			}
			else if (br == sizeof(struct chan_msg))
			{
				result = 0;
//				break;
			}
			else 
			{
				if(errno == EBADF)
				{
					result = EBADF;
//					break;
				}
				else if (errno == EAGAIN)
				{
					result = EAGAIN;
//					break;
				}
				else 
				{
					printf("Read error: %ld\n", br);
					result = -1;
//					break;
				}		
			}
		}
		else 
		{
			printf("Unknown FD ready\n");
		}
	}
	else if (ready == 0)
	{
		result = EAGAIN;
	}
	else
	{
		perror("chan_read::select");
		printf("Errno: %d\n", errno);
		result = -1;
	}

	return result;
}

#else

int chan_read(struct chan* c, struct chan_msg* m, int timeout)
{
	struct pollfd pfd;
	ssize_t br;
	int ready;
	int result;

	pfd.fd = c->fds[READ_FD];
	pfd.events = POLLIN | POLLHUP;

	for (;;)
	{
		// Fixme: Update timeout if poll is restarted
		ready = poll(&pfd, 1, timeout);
		if (ready < 0)
		{
			result = -1;
			break;
		}
		if (ready == 0)
		{
			result = EAGAIN;
			break;
		}
		if (pfd.revents & POLLIN) 
		{
#ifdef MT
                        if (pthread_mutex_lock(&c->rlock))
			{
				perror("chan_read::pthread_mutex_lock");
				// FIXME
				return -1;
			}
#endif

                        br = read(c->fds[READ_FD], m, sizeof(struct chan_msg));

#ifdef MT
                        pthread_mutex_unlock(&c->rlock);
#endif
			if (br == 0)
			{
				// The pipe is empty AND closed.
				// Strange though that POLLIN was set...
				result = EBADF;
				break;
			}
			else if (br == sizeof(struct chan_msg))
			{
				result = 0;
				break;
			}
			else 
			{
				if(errno == EBADF)
				{
					result = EBADF;
					break;
				}
				else if (errno != EAGAIN)
				{
					printf("Read error: %ld\n", br);
					result = -1;
					break;
				}
			}
		}
		else if (pfd.revents & POLLHUP)
		{
			result = EBADF;
			break;
		}
	}

	return result;
}
#endif /* USE_SELECT */

#ifdef USE_SELECT
int chan_select(struct chan** c,
		unsigned int nc,
		struct chan_msg* m,
		int timeout)
{
	fd_set fds;
	struct timeval tv;
	struct timeval* ptv;
	int ready;
	int result = EAGAIN;
	int nfds = 0;
	int possible_read = 0;

	if (timeout < 0)
	{
		ptv = NULL;
	}
	else
	{
		// Timeout is in milliseconds
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout - tv.tv_sec * 1000) * 1000;
		ptv = &tv;
	}
	
	FD_ZERO(&fds);
	
	for (unsigned int i = 0; i < nc; ++i)
	{
		FD_SET(c[i]->fds[READ_FD], &fds);
		if (c[i]->fds[READ_FD] > nfds)
		{
			nfds = c[i]->fds[READ_FD];
		}
	}
	ready = select(nfds + 1, 
		       &fds, // Read fds
		       NULL,   // write fds
		       NULL,   // error fds
		       ptv);

	if (ready == 0)
	{
		return EAGAIN;
	}
	if (ready < 0)
	{
		return -1;
	}
	for (unsigned int i = 0; i < nc; ++i)
	{
		if (FD_SET(c[i]->fds[READ_FD], &fds))
		{
			int ret = read_msg(c[i], m);
			if (ret == 0)
			{
				result = 0;
				break;
			}
			else if (ret == EAGAIN)
			{
				possible_read = 1;
				result = EAGAIN;
			}
			else if (ret == EBADF)
			{
				if (possible_read)
				{
					result = EBADF;
				}
			}
			else
			{
				if (possible_read)
				{
					result = -1;
				}
			}
		}
	}

	return result;
}

#else

int chan_select(struct chan** c, 
		unsigned int nc, 
		struct chan_msg* m, 
		int timeout)
{
	struct pollfd pfd[nc];
	ssize_t br;
	int possible_read = 0;
	int ready;
	int result = -1;
	unsigned int added = nc;

	for (unsigned int i = 0; i < nc; ++i)
	{
		pfd[i].fd = c[i]->fds[READ_FD];
		pfd[i].events = POLLIN | POLLHUP;
		pfd[i].revents = 0;
	}

	for (;;)
	{
		int done = 0;

		ready = poll(pfd, nc, timeout);
		if (ready < 0)
		{
			result = -1;
			break;
		}
		if (ready == 0)
		{
			result = EAGAIN;
			break;
		}

		for (unsigned int i = 0; i < added; ++i)
		{
			if ((pfd[i].revents & POLLIN) == 0)
			{
				continue;
			}

#ifdef MT
                        if (pthread_mutex_lock(&c[i]->rlock))
			{
				perror("chan_read::pthread_mutex_lock");
				// FIXME
				return -1;
			}
#endif

			br = read(c[i]->fds[READ_FD],
				  m, 
				  sizeof(struct chan_msg));

#ifdef MT
                        pthread_mutex_unlock(&c[i]->rlock);
#endif
			if (br == 0)
			{
				// Channel is closed. Which is strange
				continue;
			}
			else if (br == sizeof(struct chan_msg))
			{
				result = 0;
				done = 1;
				break;
			}
			else if (errno == EAGAIN)
			{
				// Someone stole our data.
				// Try to read from other channels,
				// if none succeeds, start over poll loop.
				possible_read = 1;
			}
		}

		if (done)
		{
			break;
		}
		else if (!possible_read)
		{
			result = EBADF;
			break;
		}
	}
	return result;
}
#endif /* USE_SELECT */

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

static int read_msg(struct chan* c, struct chan_msg* m)
{
	int result;
	ssize_t br;

#ifdef MT
	if (pthread_mutex_lock(&c->rlock))
	{
		perror("chan_read::pthread_mutex_lock");
		// FIXME
		return -1;
	}
#endif

	br = read(c->fds[READ_FD], m, sizeof(struct chan_msg));
#ifdef MT
	pthread_mutex_unlock(&c->rlock);
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
