#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <scut.h>
#include "chan.h"

struct wrk
{
	struct chan* c;
	int me;
	int expected;
};

static int test_read_write(void);
static int test_close_channel(void);
static int test_timeout(void);
static int test_one_select(void);
static int test_two_select(void);
static int test_one_thr(void);
static int test_two_thr(void);

int main(void)
{
	int ret;

        scut_create("Channel test");

	SCUT_ADD(test_read_write);
	SCUT_ADD(test_close_channel);
	SCUT_ADD(test_timeout);
	SCUT_ADD(test_one_select);
	SCUT_ADD(test_two_select);
	SCUT_ADD(test_one_thr);
	SCUT_ADD(test_two_thr);

        ret = scut_run(0);
                
	return ret;
}

int test_read_write(void)
{
	struct chan* c = chan_create();
	struct chan_msg s_msg;
	struct chan_msg r_msg;
	int test = 0;
	int ret;

	if (c == NULL)
	{
		printf("Could not create channel\n");
		return 1;
	}

	for (unsigned int i = 0; i < 100; ++i)
	{
		s_msg.data = (void*)((long)i);
		s_msg.len = i;	
		if (ret = chan_write(c, &s_msg), ret != 0)
		{
			printf("Write response[%d]: %d\n", i, ret);
			test = 1;
		}
	}

	for (unsigned int i = 0; i < 100; ++i)
	{
		if (ret = chan_read(c, &r_msg, 1000), ret != 0)
		{
			printf("Read response[%d]: %d\n", i, ret);
			test = 1;
			continue;
		}
		if (r_msg.data != (void*)((long)i))
		{
			printf("Wrong data in message %ld != %ld\n",
			       (long)i,
			       (long)r_msg.data);
			test = 1;
		}
		if (r_msg.len != (size_t)i)
		{
			printf("Wrong lenth in message %d != %ld\n",
			       i,
			       r_msg.len);
			test = 1;
		}
	}

	chan_destroy(c);

	return test;
}

int test_close_channel(void)
{
	struct chan* c = chan_create();
	struct chan_msg s_msg;
	struct chan_msg r_msg;
	int test = 0;
	int ret;

	for (unsigned int i = 0; i < 100; ++i)
	{
		s_msg.data = (void*)((long)i);
		s_msg.len = i;
		if (ret = chan_write(c, &s_msg), ret != 0)
		{
			printf("Write response[%d]: %d\n", i, ret);
			test = 1;
		}
	}

	chan_close(c);
	
	for (unsigned int i = 0; i < 100; ++i)
	{
		if (ret = chan_read(c, &r_msg, 1000), ret != 0)
		{
			printf("Read response[%d]: %d\n", i, ret);
			test = 1;
		}
		if (r_msg.data != (void*)((long)i))
		{
			printf("Wrong data in message %ld != %ld\n",
			       (long)i,
			       (long)r_msg.data);
			test = 1;
		}
		if (r_msg.len != (size_t)i)
		{
			printf("Wrong lenth in message %u != %ld\n",
			       i,
			       r_msg.len);
			test = 1;
		}
	}

	ret = chan_read(c, &r_msg, 1000);
	if (ret != EBADF)
	{
		printf("Expected channel closed/EBADF(%d), got %d\n",
		       EBADF, 
		       ret);
		test = 1;
	}

	chan_destroy(c);

	return test;
}

int test_timeout(void)
{
	struct chan* c = chan_create();
	struct chan_msg r_msg;
	int test = 0;
	int ret;

	ret = chan_read(c, &r_msg, 1000);
	if (ret != EAGAIN)
	{
		printf("Expected timeout/EAGAIN(%d), got %d\n",
		       EAGAIN,
		       ret);
		test = 1;
	}

	chan_destroy(c);

	return test;	
}

int test_one_select(void)
{
	struct chan* c = chan_create();
	struct chan_msg s_msg;
	struct chan_msg r_msg;
	int test = 0;
	int ret;

	for (unsigned int i = 0; i < 100; ++i)
	{
		s_msg.data = (void*)((long)i);
		s_msg.len = i;
		if (ret = chan_write(c, &s_msg), ret != 0)
		{
			printf("Write response[%d]: %d\n", i, ret);
			test = 1;
		}
	}

	for (unsigned int i = 0; i < 100; ++i)
	{
		if (ret = chan_select(&c, 1, &r_msg, 1000), ret != 0)
		{
			printf("Read response[%d]: %d\n", i, ret);
			test = 1;
		}
		if (r_msg.data != (void*)((long)i))
		{
			printf("Wrong data in message %ld != %ld\n",
			       (long)i,
			       (long)r_msg.data);
			test = 1;
		}
		if (r_msg.len != (size_t)i)
		{
			printf("Wrong lenth in message %d != %ld\n",
			       i,
			       r_msg.len);
			test = 1;
		}
	}
	chan_destroy(c);

	return test;
}

int test_two_select(void)
{
	struct chan* c1 = chan_create();
	struct chan* c2 = chan_create();
	struct chan_msg msg;
	int test = 0;
	int ret;

	for (int i = 0; i < 100; ++i)
	{
		msg.data = (void*) 567;
		msg.len = 329;
		if (ret = chan_write(c1, &msg), ret != 0)
		{
			printf("Write response[%d]: %d\n", i, ret);
			test = 1;
		}
		if (ret = chan_write(c2, &msg), ret != 0)
		{
			printf("Write response[%d]: %d\n", i, ret);
			test = 1;
		}
	}

	for (int i = 0; i < 100; ++i)
	{
		struct chan* c[2] = {c1, c2};

		if (ret = chan_select(c, 2, &msg, 100), ret != 0)
		{
			printf("Read response[%d]: %d\n", i, ret);
			test = 1;
		}
		if (msg.data != (void*)567)
		{
			printf("Wrong data in message %ld != %ld\n",
			       567L,
			       (long)msg.data);
			test = 1;
		}
		if (msg.len != 329)
		{
			printf("Wrong lenth in message %d != %ld\n",
			       329,
			       msg.len);
			test = 1;
		}
	}
	chan_destroy(c1);
	chan_destroy(c2);

	return test;
}

static void* thr_count_to_close(void* a)
{
	struct wrk* w = (struct wrk*)a;
	struct chan_msg msg;
	int ret;
	int count = 0;
	int status = 0;

	for (;;)
	{
		ret = chan_read(w->c, &msg, -1);
		if (ret == 0)
		{
			count++;
		}
                else if (ret == EAGAIN) 
                {
                        continue;
                }
		else if (ret == EBADF)
		{
			break;
		}
		else 
		{
			printf("[%d]Error reading from channel: %d\n", w->me, ret);
			status = 1;
			break;
		}

		if (msg.data != (void*)123)
		{
			printf("Wrong data in message %ld != %ld\n",
			       123L,
			       (long)msg.data);
			status = 1;
		}
		if (msg.len != 456)
		{
			printf("Wrong lenth in message %d != %ld\n",
			       456,
			       msg.len);
			status = 1;
		}
	}
	if (status)
	{
		count = -1;
	}

	return (void*)((long)count);
}

static void* thr_read_to_close(void* a)
{
	struct wrk* w = (struct wrk*)a;
	struct chan_msg msg;
	int ret;
	int count = 0;
	unsigned int start = 0;
	int status = 0;

	for (;;)
	{
		ret = chan_read(w->c, &msg, 1000);
		if (ret == 0)
		{
			count++;
		}
		else if (ret == EBADF)
		{
			break;
		}
		else 
		{
			printf("[%d]Error reading from channel: %d\n", w->me, ret);
			status = 1;
			break;
		}

		if (msg.data != (void*)((long)start))
		{
			printf("Wrong data in message %ld != %ld\n",
			       (long)start,
			       (long)msg.data);
			status = 1;
		}
		if (msg.len != (size_t)start)
		{
			printf("Wrong lenth in message %d != %ld\n",
			       start,
			       msg.len);
			status = 1;
		}
		start++;
	}

	if (w->expected != count)
	{
		printf("Read %d items, expected %d\n", count, w->expected);
		status = 1;
	}

	return (void*)((long)status);
}

int test_one_thr(void)
{
	pthread_t thr;
	struct chan* c = chan_create();
	struct wrk w;
	struct chan_msg msg;
	int ret;
	int test = 0;
	long t_ret;

	w.c = c;
	w.me = 0;
	w.expected = 10;

	if (pthread_create(&thr, NULL, &thr_read_to_close, (void*) &w))
	{
		perror("test_one_thr::pthread_create");
		return 1;
	}

	for (unsigned int i = 0; i < 10; i++)
	{
		msg.data = (void*)((long)i);
		msg.len = i;
		if (ret = chan_write(c, &msg), ret != 0)
		{
			printf("Write response: %d@%d\n", ret, i);
			test = 1;
		}
	}

	chan_close(c);
	pthread_join(thr, (void**)&t_ret);
	chan_destroy(c);
	
	if (t_ret != 0)
	{
		printf("Thread returned %ld\n", t_ret);
		test = 1;
	}

	return test;
}

int test_two_thr(void)
{
	unsigned int count = 2;
	pthread_t thr[count];
	struct wrk w[count];
	struct chan* c = chan_create();
	struct chan_msg msg;
	int ret;
	struct timeval ts;
	int test = 0;
	unsigned long sum = 0;
	unsigned long msg_count = 2470;

	for (unsigned int i = 0; i < count; ++i) {
		w[i].c = c;
		w[i].me = i;
		if (pthread_create(&thr[i], 
				   NULL, 
				   &thr_count_to_close,
				   (void*) &w[i]))
		{
			perror("test_one_thr::pthread_create");
			return 1;
		}
	}

	for (unsigned int i = 0; i < (unsigned int)msg_count; i++)
	{
		msg.data = (void*)123;
		msg.len = 456;
		gettimeofday(&ts, NULL);
		if (ret = chan_write(c, &msg), ret != 0)
		{
			printf("Write response: %d@%u\n", ret, i);
		}
	}

	chan_close(c);

	for (unsigned int i = 0; i < count; ++i)
	{
		long t_ret;

		pthread_join(thr[i], (void**)&t_ret);
		
		if (t_ret < 0)
		{
			printf("Tread failed\n");
			test = 1;
		}
		
		sum += t_ret;
	}
	
	if (sum != msg_count)
	{
		printf("Thread read %ld messages, %ld expected\n",
		       sum,
		       msg_count);
	}

	chan_destroy(c);

	return test;
}

