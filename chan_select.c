#include "chan.h"
#include "chan_def.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

int chan_read(struct chan* c, struct chan_msg* m, int timeout)
{
	fd_set fds;
	struct timeval tv;
	struct timeval* ptv;
	int ready;
	int result = EAGAIN;

	/* Timeout is in milliseconds */
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
		       &fds, /* read fds */
		       NULL, /* write fds */
		       NULL, /* error fds */
		       ptv);
	if (ready > 0)
	{

		if (FD_ISSET(c->fds[READ_FD], &fds))
		{
                        result = chan_read_msg(c, m);
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

	if (timeout < 0)
	{
		ptv = NULL;
	}
	else
	{
		/* Timeout is in milliseconds */
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

        for (;;) 
	{
                int done = 0;
                int possible_read = 0;

                /* TODO decrement timeout */
                ready = select(nfds + 1, 
                               &fds, /* read fds */
                               NULL, /* write fds */
                               NULL, /* error fds */
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
                                int ret = chan_read_msg(c[i], m);

                                if (ret == 0)
                                {
                                        result = 0;
                                        done = 1;
                                        break;
                                }
                                else if (ret == EBADF)
                                {
                                        /* channel is closed */
                                        continue;
                                }
                                else if (ret == EAGAIN)
                                {
                                        /* Data got stolen by 
                                           some other reader. */
                                        possible_read = 1;
                                }
                        }
                }
                
                if (done)
                {
                        break;
                } 
                else if(!possible_read)
                {
                        result = EBADF;
                        break;
                }
        }

	return result;
}
