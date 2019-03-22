#include "threadpool.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static void *threadpool_routine(void *arg) {
  struct threadpool *tp = (struct threadpool*)arg;
  struct job *curjob = NULL;
  for(;;) {
    pthread_mutex_lock(&tp->lock);
    // 如果队列为空，且线程池被通知关闭，就退出线程
    if (tp->workq == NULL && tp->threadpool_close) {
      pthread_mutex_unlock(&tp->lock);
      // 条件变量 quit 成立
      pthread_cond_signal(&tp->quit);
      // 线程退出
      pthread_exit(NULL);
    }

    // 等待队列不空，就从队列中摘下队头准备处理
    while(tp->workq == NULL) {
      // 如果队列就绪（空队列），所有线程都进入等待队列，让出 cpu
      pthread_cond_wait(&tp->ready, &tp->lock);
    }
    curjob = tp->workq; // 摘下队头
    tp->workq = curjob->next;
    pthread_mutex_unlock(&tp->lock);
    // 开始执行任务
    curjob->job_routine(curjob->arg);
    // 任务执行完后释放掉任务结构体
    free(curjob);
    curjob = NULL;
  }
}

// 初始化 thread_num 个线程
struct threadpool* threadpool_init(int thread_num) {
  int err, i;
  // 分配线程池结构体，待会儿返回
  struct threadpool *tp = (struct threadpool*)malloc(sizeof(struct threadpool));
  // 初始化互斥量
  err = pthread_mutex_init(&tp->lock, NULL);
  if (err) PERR(err, "pthread_mutex_init");
  // 初始化队列就绪条件，就绪是指队列中已经有任务可以拿出来了。
  err = pthread_cond_init(&tp->ready, NULL);
  if (err) PERR(err, "pthread_cond_init ready");
  // 初始化队列是否可以退出条件，只有此条件成立，才“有可能”正常退出。
  err = pthread_cond_init(&tp->quit, NULL);
  if (err) PERR(err, "pthread_cond_init quit");

  // 初始化其它成员
  tp->workq = NULL;
  tp->threadpool_close = 0;
  tp->tids = (pthread_t*)malloc(sizeof(pthread_t) * thread_num);
  tp->thread_num = thread_num;
  
  // 创建线程
  for (i = 0; i < thread_num; ++i) {
    err = pthread_create(tp->tids + i, NULL, threadpool_routine, (void*)tp);
    if (err) PERR(err, "pthread_create");
  }

  return tp;
}


// 添加任务，任务就是一个你要执行的函数。
int threadpool_add_job(struct threadpool* tp, void (*job_routine)(void* arg), void* arg) {
  pthread_mutex_lock(&tp->lock);
  // 如果线程池被通知关闭了，就不能再添加任务
  if (tp->threadpool_close) {
    pthread_mutex_unlock(&tp->lock);
    return -1;
  }
  // 分配任务结构体
  struct job *worker = (struct job*)malloc(sizeof(struct job));
  worker->job_routine = job_routine;
  worker->arg = arg;
  worker->next = tp->workq;
  tp->workq = worker;
  pthread_mutex_unlock(&tp->lock);
  // 任务队列已就绪，唤醒至少一个线程
  pthread_cond_signal(&tp->ready);
  return 0;
}

// 关闭线程池
void threadpool_destroy(struct threadpool* tp) {
  int i;
  pthread_mutex_lock(&tp->lock);
  // 置关闭标记为 true
  tp->threadpool_close = 1;
  // 等待队列为空
  while(tp->workq != NULL) {
    pthread_cond_wait(&tp->quit, &tp->lock);
  }
  pthread_mutex_unlock(&tp->lock);
  // 等待所有线程完全退出
  for (i = 0; i < tp->thread_num; ++i) {
    pthread_join(tp->tids[i], NULL);
  }

  // 释放线程 id 数组
  free(tp->tids);
  // 销毁申请的资源
  pthread_cond_destroy(&tp->ready);
  pthread_cond_destroy(&tp->quit);
  pthread_mutex_destroy(&tp->lock);
  free(tp);
}









