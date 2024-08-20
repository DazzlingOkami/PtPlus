/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2024, DazzlingOkami (https://dazzlingokami.github.io/)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * This file is part of the PtPlus library.
 * 
 */

#ifndef _PT_PLUS_H
#define _PT_PLUS_H
#include "protothreads/pt.h"
#include "protothreads/pt-sem.h"
#include "list.h"

#define PT_PLUS_DELAY_SUPPORT 1

#if defined(PT_PLUS_DELAY_SUPPORT) && (PT_PLUS_DELAY_SUPPORT == 1)
#include "protothreads/timer.h"
#endif

typedef struct
{
    struct list_node list;
    PT_THREAD((*task)(struct pt *pt));
    struct pt pt;
    #if defined(PT_PLUS_DELAY_SUPPORT) && (PT_PLUS_DELAY_SUPPORT == 1)
    struct timer periodic;
    #endif
} pt_item_t;

extern struct list_node pt_pool;

#define _PT_LINK(AA,BB) AA##BB
#define PT_LINK(AA,BB) _PT_LINK(AA,BB)

/**
 * Run pt scheduler.
 * PT_TASK_SCHEDULE() executes an infinite loop.
 * 
 * Example usage:
 * @code{c}
 * int main(void){
 *     //...
 *     for(;;){
 *         PT_TASK_SCHEDULE();
 *     }
 *     return 0;
 * }
 * @endcode
 */
#define PT_TASK_SCHEDULE() pt_task_schedule();
void pt_task_schedule(void);

#if defined(PT_PLUS_DELAY_SUPPORT) && (PT_PLUS_DELAY_SUPPORT == 1)
/**
 * Query the next scheduled time.
 * Used to enable devices to run in low-power mode.
 * 
 * Example usage:
 * @code{c}
 *  int main(void){
 *      //...
 *      while(1){
 *          PT_TASK_SCHEDULE();
 *          clock_time_t idle_time = PT_TASK_IDLE_TIME();
 *          if(idle_time > 0){
 *              // Calling system delay functions
 *              sleep(idle_time);
 *          }
 *      }
 *  }
 * @endcode
 */
#define PT_TASK_IDLE_TIME() pt_task_idle_time()
clock_time_t pt_task_idle_time(void);
#endif

/** 
 * Create a new pt task.
 * 
 * Example usage:
 * @code{c}
 * PT_THREAD(iwdg_task(struct pt *pt)){
 *      static struct timer periodic_timer;
 *      PT_BEGIN(pt);
 *      timer_set(&periodic_timer, 100);
 *      while(1){
 *          HAL_IWDG_Refresh(&hiwdg);
 *          PT_WAIT_UNTIL(pt, timer_expired(&periodic_timer));
 *          timer_reset(&periodic_timer);
 *      }
 *      PT_END(pt);
 * }
 * 
 * int main(void){
 *     PT_TASK_RUN(iwdg_task);
 *     //...
 *     for(;;){
 *         PT_TASK_SCHEDULE();
 *     }
 *     return 0;
 * }
 * @endcode
 */
#define PT_TASK_RUN(func)                            \
    do                                               \
    {                                                \
        static pt_item_t PT_LINK(pt_,func);          \
        PT_LINK(pt_,func).task = func;               \
        PT_INIT(&(PT_LINK(pt_,func).pt));            \
        list_add_head(&pt_pool, &(PT_LINK(pt_,func).list)); \
    } while (0)

#if defined(PT_PLUS_DELAY_SUPPORT) && (PT_PLUS_DELAY_SUPPORT == 1)
/** 
 * Delay a pt task for a given number of millisecond.
 * 
 * Example usage:
 * @code{c}
 * PT_THREAD(test_task(struct pt *pt)){
 *      PT_BEGIN(pt);
 *      
 *      while(1){
 *          // Output "hello world!" every 1500 milliseconds.
 *          PT_TASK_DELAY(pt, 1500);
 *          printf("hello world!\r\n");
 *      }
 *      PT_END(pt);
 * }
 * 
 * int main(void){
 *     PT_TASK_RUN(test_task);
 *     //...
 *     for(;;){
 *         PT_TASK_SCHEDULE();
 *     }
 *     return 0;
 * }
 * @endcode
 */
#define PT_TASK_DELAY(pt_p, ms)                                            \
    do                                                                     \
    {                                                                      \
        timer_set(&(containerof(pt_p, pt_item_t, pt)->periodic), ms);      \
        PT_WAIT_UNTIL(pt,                                                  \
            timer_expired(&(containerof(pt_p, pt_item_t, pt)->periodic))); \
    } while (0)
#endif

/** 
 * The number of tasks that the pt is currently managing.
 */
#define PT_TASK_NUMS() list_length(&pt_pool)

/**
 * Declare a pt thread in a simple way.
 * 
 * Example usage:
 * @code{c}
 * PT_THREAD_DECL(thread1, {
 *     while(1){
 *         printf("hello pt!\r\n");
 *         PT_TASK_DELAY(pt, 100);
 *     }
 * });
 * 
 * // PT_TASK_RUN(thread1);
 * @endcode
 */
#define PT_THREAD_DECL(name, body) \
    PT_THREAD(name(struct pt *pt)){PT_BEGIN(pt);PT_YIELD(pt);body;PT_END(pt);}

/** 
 * Asynchronous execution. Only used in coroutine function.
 * 
 * Example usage:
 * @code{c}
 * PT_THREAD_DECL(invok_test, {
 *     static int cnt;
 * 
 *     cnt = 0;
 * 
 *     PT_INVOK({
 *         static int i;
 *         for(i = 0; i < 10; i++){
 *             printf("async invok %d\r\n", i);
 *             cnt += i;
 *             PT_TASK_DELAY(pt, 1000);
 *         }
 *         // Automatically exit this asynchrony at end of PT_INVOK();
 *     });
 * 
 *     PT_INVOK({
 *         while(1){
 *             printf("hello invok, cnt = %d\r\n", cnt);
 *             PT_TASK_DELAY(pt, 300);
 *         }
 *     });
 * });
 * 
 * // PT_TASK_RUN(invok_test);
 * @endcode
 */
#define PT_INVOK(body)                             \
    do                                             \
    {                                              \
        static pt_item_t pt_invok;                 \
        static char pt_anchor;                     \
        pt_invok.task =                            \
            containerof(pt, pt_item_t, pt)->task;  \
        list_add_head(&pt_pool, &(pt_invok.list)); \
        pt_anchor = 0;                             \
        LC_SET(pt_invok.pt.lc);                    \
        if (pt_anchor)                             \
        {                                          \
            body;                                  \
            PT_EXIT(pt);                           \
        }                                          \
        pt_anchor = 1;                             \
    } while (0)

#endif

#define PT_SEM_SIGNAL_FROM_ISR(s) ++(s)->count

#if defined(PT_PLUS_DELAY_SUPPORT) && (PT_PLUS_DELAY_SUPPORT == 1)

/**
 * Wait for a semaphore within the specified timeout period.
 * 
 * Compared to PT-SEM-WAIT(), it supports a timeout mechanism.
 * 
 * \param pt_p (struct pt *) A pointer to the protothread (struct pt) in
 * which the operation is executed.
 * 
 * \param s (struct pt_sem *) A pointer to the pt_sem struct
 * representing the semaphore
 * 
 * \param ms (clock_time_t) A number representing the timeout
 * 
 * \param ret (int *) A pointer to the integer used to save the return value.
 * Obtained semaphore successfully if the return value is equal to 0
 * 
 * Example usage:
 * @code{c}
 *  PT_THREAD(test_task_A(struct pt *pt)){
 *      PT_BEGIN(pt);
 *      static int i;
 *      for(i = 0; i < 10; i++){
 *          PT_TASK_DELAY(pt, 1000);
 *          PT_SEM_SIGNAL(pt, &sem);
 *      }
 *      PT_END(pt);
 *  }
 *  
 *  PT_THREAD(test_task_B(struct pt *pt)){
 *      PT_BEGIN(pt);
 *      static int err_cnt = 0;
 *      while(1){
 *          int ret; // this veriable allow be non-static
 *          PT_SEM_WAIT_TIMEOUT(pt, &sem, 2000, &ret);
 *          if(ret == 0){
 *              printf("Obtained semaphore! - %d\r\n", clock_time());
 *          }else{
 *              printf("Obtained semaphore timeout - %d\r\n", clock_time());
 *              err_cnt++;
 *              if(err_cnt >= 3){
 *                  PT_EXIT(pt);
 *              }
 *          }
 *      }
 *      PT_END(pt);
 *  }
 * @endcode{c}
 * 
 * \hideinitializer
 */
#define PT_SEM_WAIT_TIMEOUT(pt_p, s, ms, ret)                              \
    do                                                                     \
    {                                                                      \
        timer_set(&(containerof(pt_p, pt_item_t, pt)->periodic), ms);      \
        PT_WAIT_UNTIL(pt_p, ((s)->count > 0) ||                            \
            timer_expired(&(containerof(pt_p, pt_item_t, pt)->periodic))); \
        if ((s)->count > 0)                                                \
        {                                                                  \
            (s)->count--;                                                  \
            *(ret) = 0;                                                    \
        }                                                                  \
        else                                                               \
        {                                                                  \
            *(ret) = 1;                                                    \
        }                                                                  \
    } while (0)
#endif
