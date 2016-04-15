# Thread safe inter-process channel implementation for C

## Building

Builds properly with C99. Other versions of C is not tested.

## Implementation

Implemented as a wrapper around a channel created via `pipe(2)`, with
a user friendly interface. Supports basic operations such as read a
message, write a message, selecting on multiple channels and more
elaborated features such as fan in and fan out. 

## Message type

A messate is a simple C struct:

```
struct chan_msg
{
	void* data;
	size_t len;
};
```

## Delivery of a messge

When a message is being put on a channel for transmission, *only* the
message struct is being copied. Any data referenced by the data
pointer is not copied. Ownership of data delivered over a channel has
to be defined by the application using it.

## Performance

On an Intel x3450@2.67GHz running Solaris 11.3 it is capable of
delivering ~1M messages per second. 

## Example program 

A simple program (error handling is omitted), tiny.c

``` 
#include <stdio.h>
#include <errno.h>
#include "chan.h"

int main(void)
{
        struct chan* c = chan_create();
        int num_msg = 100;
        struct chan_msg m = {.data = NULL, .len = 0};
        int ret;

        for (int i = 0; i < num_msg; ++i)
        {
                chan_write(c, &m);
        }
        chan_close(c);
        num_msg = 0;
        for (;;)
        {
                ret = chan_read(c, &m, 10);
                if (ret == 0)
                {
                        num_msg++;
                }
                else if (ret == EAGAIN)
                {
                        printf("Read timeout on channel\n");
                }
                else if (ret == EBADF)
                {
                        printf("Channel was closed\n");
                        break;
                }
                else if (ret == -1)
                {
                        printf("Error occured on channel\n");
                        break;
                }
        }
        chan_destroy(c);
        printf("Read %d messages\n", num_msg);
        return 0;
}
```

Compile tiny.c and run:

```
$ gcc -W -Wall -pedantic -std=c99 -DMT_SAFE -D_POSIX_C_SOURCE=200112L \
      tiny.c chan_poll.c chan.c lock.c
$ ./a.out
Channel was closed
Read 100 messages
``` 

## Example program with fan-out and select

Creates a source channel, with two target channels that both receives
the messages sent to the source channel. Read from both target
channels with chan_select:

``` 
#include <stdio.h>
#include <errno.h>
#include "chan.h"

int main(void)
{
        struct chan* c = chan_create();
        struct chan* tgt[2];
        int num_msg = 100;
        struct chan_msg m = {.data = NULL, .len = 0};
        int ret;

        tgt[0] = chan_create();
        tgt[1] = chan_create();

        chan_fan_out(tgt, 2, c);

        for (int i = 0; i < num_msg; ++i)
        {
                chan_write(c, &m);
        }
        chan_close(c);
        num_msg = 0;
        for (;;)
        {
                ret = chan_select(tgt, 2, &m, 10);
                if (ret == 0)
                {
                        num_msg++;
                }
                else if (ret == EAGAIN)
                {
                        printf("Read timeout on channel\n");
                        break;
                }
                else if (ret == EBADF)
                {
                        printf("Channel was closed\n");
                        break;
                }
                else if (ret == -1)
                {
                        printf("Error occured on channel\n");
                        break;
                }
        }
        chan_destroy(c);
        printf("Read %d messages\n", num_msg);
        return 0;
}
``` 

Build and run fout.c
```
$ gcc -W -Wall -pedantic -std=c99 -DMT_SAFE -D_POSIX_C_SOURCE=200112L \
      fout.c chan_poll.c chan.c lock.c
$ ./a.out
Read timeout on channel
Read 200 messages
``` 
