#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <pthread.h>
#define PERR(err, msg) do { errno = err; perror(msg); exit(-1); } while(0)

// Job queue
struct job {
  void (*job_routine)(void* arg);
  void* arg;
  struct job* next;
};


struct threadpool {
  pthread_mutex_t lock;
  pthread_cond_t ready; // 队列是否空
  pthread_cond_t quit; // 通知退出 
  struct job *workq; // 队头
  int threadpool_close; // 线程池被关闭
  pthread_t *tids;
  int thread_num;
};


struct threadpool* threadpool_init(int thread_num);
int threadpool_add_job(struct threadpool* tp, void (*job_routine)(void* arg), void* arg);
void threadpool_destroy(struct threadpool* tp);
#endif
