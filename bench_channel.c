#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include "chan.h"

#define USEC(ts) (ts.tv_sec * 1000000 + ts.tv_usec)

void bench_consume(int);
void bench_select1(int);
void* thr_wrk_consume(void*);

struct wrk
{
	struct chan* c;
	int me;
	int expected;
};

int main(void)
{
	for (int num_threads = 2; num_threads < 10; num_threads += 2) {
		printf("Benchmark with 1 producer, %d consumers\n", 
		       num_threads);
		bench_consume(num_threads);
	}

	for (int num_threads = 2; num_threads < 10; num_threads += 2) {
		printf("Benchmark with 1 producer, %d consumers\n", 
		       num_threads);
		bench_select1(num_threads);
	}
}

void* thr_wrk_consume(void* a)
{
	struct wrk* w = (struct wrk*)a;
	struct chan_msg msg;
	int ret;
	int count = 0;

	while(1)
	{
		ret = chan_read(w->c, &msg, -1);
		if (ret == 0)
		{
			size_t r = (size_t)msg.data;
			
			if (msg.len != r)
			{
				printf("Incosistent message found\n");
			}

			count++;
		}
		else if (ret == EBADF)
		{
			//printf("[%d]Channel is closed\n", w->me);
			break;
		}
		else if (ret == EAGAIN)
		{
			continue;
		}
		else 
		{
			printf("[%d]Error reading from channel: %d\n", w->me, ret);
			break;
		}			
	}

	return (void*) (long)count;
}

void bench_consume(int count)
{
	unsigned int msg_count = 1000000;
	pthread_t thr[count];
	struct wrk w[count];
	struct chan* c = chan_create();
	struct chan_msg msg;
	int ret;
	struct timeval ts;
	long start;
	long stop;
	unsigned int thr_msg = 0;
	double dur_s;

	for (int i = 0; i < count; i++)
	{
		w[i].c = c;
		w[i].me = i;
		if (pthread_create(&thr[i], NULL, &thr_wrk_consume, (void*) &w[i]))
		{
			perror("bench_consume::pthread_create");
			return;
		}
	}
	gettimeofday(&ts, NULL);
	start = USEC(ts);
	for (unsigned int i = 0; i < msg_count; i++) 
	{
		msg.data = (void*)(long)i;
		msg.len = i;
		if (ret = chan_write(c, &msg), ret != 0)
		{
			printf("Write response: %d@%d\n", ret, i);
		}
	}
	//printf("Closing channel\n");
	chan_close(c);
	for (int i = 0; i < count; i++)
	{
		void* t_ret;
		unsigned int tmp;

		pthread_join(thr[i], &t_ret);
		tmp = (unsigned int)(long)t_ret;
		//printf("Thread %d read %d msg\n", i, tmp);
		thr_msg += tmp;
	}
	gettimeofday(&ts, NULL);
	stop = USEC(ts);
	printf("Total number of processed messages: %d\n", thr_msg);
	
	dur_s = (double)(stop - start);
	dur_s /= 1000000;
	printf("Processed %d messages in %fs\n", thr_msg, dur_s);
	printf("Processed %f message/second\n", thr_msg / dur_s);
}

void bench_select1(int count)
{
	
}
