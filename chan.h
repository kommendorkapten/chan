#ifndef __CHAN_H__
#define __CHAN_H__

#include <sys/types.h>

struct chan_msg
{
	void* data;
	size_t len;
};
struct chan;

/**
 * Create a uni directona channel.
 * @return the channel.
 */
extern struct chan* chan_create(void);

/**
 * Destroyes the channel.
 * Any behavior of subsequent read/write is undefined.
 */
extern void chan_destroy(struct chan*);

/**
 * Closes the read end of the channel.
 */
extern void chan_close(struct chan*);

/**
 * Reads from the channel.
 * @param the channel.
 * @param a pointer to chan_msg struct to store the message in.
 * @param timeout in milliseconds. 0 for polling, -1 for blocking.
 * @return 0 on success.
 *         EBADF if the channel is closed.
 *         EAGAIN if the timeout was reached.
 *         -1 on other error, see errno for more information.
 */
extern int chan_read(struct chan*, struct chan_msg*, int);

/**
 * Read a single message from a multiple set of channels.
 * At most one message is read. If multiple channels are ready
 * the first read channel will be read from.
 * @param pointer to channels.
 * @param the number of channels to wait for.
 * @param a pointer to chan_msg struct to store the message in.
 * @param timeout in milliseconds.
 * @return 0 on success.
 *         EBADF if the channel is closed.
 *         EAGAIN if the timeout was reached.
 *         -1 on other error, see errno for more information.
 */
extern int chan_select(struct chan**, unsigned int, struct chan_msg*, int);

/**
 * Write a message onto a channel.
 * When writing a message, the data referenced by the message is *not* copied.
 * It is up to the client to defined the ownership of any referenced data.
 * @param the channel.
 * @param the message to write.
 * @return 0 if successful.
 *         -1 otherwise, se errno for more information.
 */
extern int chan_write(struct chan*, struct chan_msg*);

#endif /* __CHAN_H__ */
