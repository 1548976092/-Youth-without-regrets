#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

int gTestCnt = 1000;

int test(int tIndex, int cnt)
{
    char *pDst = NULL;
    char *pSrc = NULL;
    int   len  = 2048 * 1024;
    int   i = 0;
    struct timeval t1, t2;


    pDst = (char *)malloc(len);
    pSrc = (char *)malloc(len);

    if (!pDst || !pSrc)
    {
	printf("malloc failed.\n");
	return -1;
    }

    gettimeofday(&t1, NULL);

    for(i = 0; i < cnt; i++)
    {
	memset(pSrc, i&0xff, len);
	memcpy(pDst, pSrc, len);
    }

    gettimeofday(&t2, NULL);

    printf("thread[%d] test ok, time=%ldms, cnt=%d.\n", tIndex, ((t2.tv_sec - t1.tv_sec)*1000000 + (t2.tv_usec - t1.tv_usec))/1000, cnt);

    free(pDst);
    free(pSrc);

    return 0;
}

static void *thread_func(void *arg)
{
    int tid = *((int *)arg);

    test(tid, gTestCnt);

    return NULL;
}


int main(int argc, char **argv)
{
    int       threadCnt = 1;
    int       threadIdx = 0;
    int       threadIdxArg[4];
    pthread_t tid[4];
    cpu_set_t cpu_info;

    if (argc >= 2)
    {
	threadCnt = atoi(argv[1]);
        if (threadCnt > 4 || threadCnt <= 0)
	{
	    printf("invalid threadCnt:%d.\n", threadCnt);
	    return -1;
	}
    }
    
    if (argc >= 3)
    {
        gTestCnt = atoi(argv[2]);
        if (gTestCnt <= 0)
        {
            printf("invalid gTestCnt:%d.\n", gTestCnt);
            return -1;
        }
    }

    for(threadIdx = 0; threadIdx < threadCnt; threadIdx++)
    {
        threadIdxArg[threadIdx] = threadIdx;
	pthread_create(&tid[threadIdx], NULL, thread_func, (void *)&threadIdxArg[threadIdx]);
        
        CPU_ZERO(&cpu_info);
        CPU_SET(threadIdx, &cpu_info);
        pthread_setaffinity_np(tid[threadIdx], sizeof(cpu_set_t), &cpu_info);
    }

    for(threadIdx = 0; threadIdx < threadCnt; threadIdx++)
    {
        pthread_join(tid[threadIdx], NULL);    
    }

    return 0;
}
