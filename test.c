#include <stdio.h>
#include "pt_plus.h"
#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep_ms(ms) usleep(ms * 1000)
#endif

#if defined(TEST) && (TEST == 1)
struct pt_sem sem;

PT_THREAD(test_task_A(struct pt *pt)){
    PT_BEGIN(pt);
    static int i;
    for(i = 0; i < 10; i++){
        PT_TASK_DELAY(pt, 1000);
        printf("send semaphore - %d\r\n", clock_time());
        PT_SEM_SIGNAL(pt, &sem);
    }
    PT_END(pt);
}

PT_THREAD(test_task_B(struct pt *pt)){
    PT_BEGIN(pt);
    static int err_cnt = 0;
    while(1){
        int ret;
        PT_SEM_WAIT_TIMEOUT(pt, &sem, 2000, &ret);
        if(ret == 0){
            printf("Obtained semaphore! - %d\r\n", clock_time());
        }else{
            printf("Obtain semaphore timeout - %d\r\n", clock_time());
            err_cnt++;
            if(err_cnt >= 3){
                PT_EXIT(pt);
            }
        }
    }
    PT_END(pt);
}

int main(void){
    PT_SEM_INIT(&sem, 0);
    PT_TASK_RUN(test_task_A);
    PT_TASK_RUN(test_task_B);

    while(PT_TASK_NUMS() > 0){
        PT_TASK_SCHEDULE();
        clock_time_t idle_time = PT_TASK_IDLE_TIME();
        if(idle_time > 0){
            // Calling system delay functions
            // printf("idle time - %d\r\n", idle_time);
            sleep_ms(idle_time);
        }
    }
    return 0;
}
#endif
