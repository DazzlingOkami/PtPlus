#include <stdio.h>
#include "pt_plus.h"

struct pt_sem sem;

PT_THREAD(test_task_A(struct pt *pt)){
    PT_BEGIN(pt);

    while(1){
        PT_TASK_DELAY(pt, 1000);
        PT_SEM_SIGNAL(pt, &sem);
    }
    PT_END(pt);
}

PT_THREAD(test_task_B(struct pt *pt)){
    PT_BEGIN(pt);

    PT_SEM_INIT(&sem, 0);

    while(1){
        PT_SEM_WAIT(pt, &sem);
        printf("hello world!\r\n");
    }
    PT_END(pt);
}

int main(void){
    PT_TASK_RUN(test_task_A);
    PT_TASK_RUN(test_task_B);
    for(;;){
        PT_TASK_SCHEDULE();
    }
    return 0;
}
