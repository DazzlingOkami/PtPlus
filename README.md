## PT Plus
This is a lightweight coroutine scheduling framework. It supports the creation, scheduling, destruction, suspension, recovery, waiting, timeout, and other operations of coroutines. It is based on Protothreads implementation and extends to more useful user interfaces.

## Feature
- Support dynamic creation and destruction of coroutines
- Support regular delay interfaces
- Support asynchronous execution
- Extremely low ram usage
- Non-preemptive scheduling
- Static memory management
- Support low-power mode

## Usage
Detailed documentation can be found in the comments of pt_plus.h.

The interface of PT Plus is mainly implemented using macros, which are compatible with most compilers, and it is best to use compilers with C99 or higher standards.

To port to other operating platforms, it is necessary to implement the clock_time() function in the clock_arch.c file, which returns the current system time and serves as the time reference for coroutine scheduling. Of course, this is not necessary. When PT_PLUS_DELAY_SUPPORT=0, all time related interfaces can be closed.

This framework is designed based on non preemptive mode. If semaphores or tasks are used in interrupts, protection measures need to be taken by oneself. Using APIs cannot guarantee interrupt security.

In most cases, the use of local variables within a coroutine is prohibited, and all variables in the coroutine function body must be declared as static. Unless the variable has a very short lifecycle and has not undergone coroutine scheduling during its lifecycle.

There are some limitations to using low-power mode. When using the PT_TWAIT_UNTIL(), PT_TWAIT_WHILE(), and PT_SEM_WAIT() interfaces, the coroutine will enter a waiting state without timeout, and the system will not be able to evaluate the global low-power waiting time. Therefore, when using these interfaces, PT_TASK_IDLE_TIME() cannot be used to evaluate the duration of sleep.

## Example
1. Create a coroutine that outputs "hello world!" every 1500ms
```c
PT_THREAD(test_task(struct pt *pt)){
    PT_BEGIN(pt);
    
    while(1){
        // Output "hello world!" every 1500 ms.
        PT_TASK_DELAY(pt, 1500);
        printf("hello world!\r\n");
    }
    PT_END(pt);
}

int main(void){
    PT_TASK_RUN(test_task);
    //...
    for(;;){
        PT_TASK_SCHEDULE();
    }
    return 0;
}
```

2.Create a coroutine, output the counter value every 1 second, and delete the coroutine after outputting it to 10
```c
PT_THREAD(test_task(struct pt *pt)){
    PT_BEGIN(pt);

    // Only static variables can be used in PT_THREAD.
    static int i = 0;

    while(1){
        PT_TASK_DELAY(pt, 1000);
        i++;
        printf("%d\r\n", i);
        if(i == 10){
            // It will be automatically deleted after exit.
            PT_EXIT(pt);
        }
    }
    PT_END(pt);
}

int main(void){
    PT_TASK_RUN(test_task);
    while(PT_TASK_NUMS() > 0){
        PT_TASK_SCHEDULE();
    }
    return 0;
}
```

3.Create two coroutines, coroutine A outputs a semaphore, coroutine B waits for the semaphore and outputs "Obtained semaphore!"
*** Note: Non timeout wait signals cannot be used in low-power mode ***
```c
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

    while(1){
        PT_SEM_WAIT(pt, &sem);
        printf("Obtained semaphore!\r\n");
    }
    PT_END(pt);
}

int main(void){
    PT_SEM_INIT(&sem, 0);
    PT_TASK_RUN(test_task_A);
    PT_TASK_RUN(test_task_B);
    for(;;){
        PT_TASK_SCHEDULE();
    }
    return 0;
}
```

4.Create a coroutine that executes two asynchronous actions simultaneously within the coroutine
```c
PT_THREAD_DECL(invok_test, {
    static int cnt;
    cnt = 0;

    PT_INVOK({
        static int i;
        for(i = 0; i < 10; i++){
            printf("async invok %d\r\n", i);
            cnt += i;
            PT_TASK_DELAY(pt, 1000);
        }
        // Automatically exit this asynchrony at end of PT_INVOK();
    });

    PT_INVOK({
        while(cnt < 5){
            printf("hello invok, cnt = %d\r\n", cnt);
            PT_TASK_DELAY(pt, 300);
        }
    });
});

int main(void){
    PT_TASK_RUN(invok_test);
    while(PT_TASK_NUMS() > 0){
        PT_TASK_SCHEDULE();
    }
    return 0;
}
```

5.Can wait for semaphore timeout (low-power mode)
```c
struct pt_sem sem;

PT_THREAD(test_task_A(struct pt *pt)){
    PT_BEGIN(pt);
    static int i;
    for(i = 0; i < 10; i++){
        PT_TASK_DELAY(pt, 1000);
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
            printf("Obtained semaphore timeout - %d\r\n", clock_time());
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
            sleep(idle_time);
        }
    }
    return 0;
}
```

## API
| Function                                  | description |
|-------------------------------------------|-------------|
| PT_TASK_SCHEDULE()                        | Run coroutine schedule |
| PT_TASK_IDLE_TIME()                       | Get the duration of sleep |
| PT_TASK_NUMS()                            | Get the number of coroutines |
| PT_TASK_RUN(func)                         | Create a coroutine |
| PT_THREAD_DECL(name, body)                | Declare a coroutine |
| PT_TASK_DELAY(pt, ms)                     | Delay coroutine |
| PT_INVOK(body)                            | Coroutine asynchronous execution |
| PT_YIELD(pt)                              | Coroutine yield |
| PT_EXIT(pt)                               | Coroutine exit |
| PT_WAIT_UNTIL(pt, condition)              | Wait until the condition is true |
| PT_WAIT_WHILE(pt, cond)                   | Wait while the condition is true |
| clock_time()                              | Get system time |
| PT_SEM_INIT(sem, count)                   | Semaphore initialization |
| PT_SEM_WAIT(pt, sem)                      | Wait for semaphore |
| PT_SEM_WAIT_TIMEOUT(pt, sem, timeout, ret)| Wait for semaphore with timeout |
| PT_SEM_SIGNAL(pt, sem)                    | Signal semaphore |


## Compile & Test
```bash
make test
```

## Related
1. [Protothreads](https://github.com/gburd/pt)
