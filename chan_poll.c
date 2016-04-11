#include "chan.h"
#include "chan_def.h"
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>

int chan_read(struct chan* c, struct chan_msg* m, int timeout)
{
	struct pollfd pfd;
	int ready;
	int result;

	pfd.fd = c->fds[READ_FD];
	pfd.events = POLLIN | POLLHUP;

        // Fixme: Update timeout if poll is restarted
        ready = poll(&pfd, 1, timeout);
        if (ready < 0)
        {
                result = -1;
        }
        if (ready == 0)
        {
                result = EAGAIN;
        }
        if (pfd.revents & POLLIN) 
        {
                result = read_msg(c, m);
        }
        else if (pfd.revents & POLLHUP)
        {
                result = EBADF;
        }

	return result;
}

int chan_select(struct chan** c, 
		unsigned int nc, 
		struct chan_msg* m, 
		int timeout)
{
	struct pollfd pfd[nc];
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
                int possible_read = 0;

                // TODO decrement timeout
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
                        int ret;

			if ((pfd[i].revents & POLLIN) == 0)
			{
				continue;
			}

                        ret = read_msg(c[i], m);
                        
                        if (ret == 0)
                        {
                                done = 1;
                                result = 0;
                                break;
                        }
                        else if (ret == EBADF)
			{
				// Channel is closed. Which is strange
				continue;
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
