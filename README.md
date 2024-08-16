## PT Plus
这是一个轻量级的协程调度框架，支持协程的创建、调度、销毁、挂起、恢复、等待、超时等操作。它基于Protothreads实现，扩展出了更高效的使用接口。

## 特性
- 支持创建和销毁协程
- 支持协程延时
- 支持异步执行
- 极低的内存使用
- 非抢占式调度
- 静态内存管理
- 支持低功耗模式

## 示例
1. 创建一个协程，每隔1500ms输出"hello world!":
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

2.创建一个协程，每隔1秒输出计数器值，输出到10后删除协程：
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

3.创建两个协程，协程A输出一个信号量，协程B等待信号量，输出"Obtained semaphore!":
*** 注意：非超时的等待信号量不可用在低功耗模式下 ***
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

4.创建一个协程，协程内同时执行两个异步动作：
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

5.可以超时的等待信号量(低功耗模式)
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

## 用法
详细的文档可在pt_plus.h的注释中找到。

pt plus的接口基本采用宏实现，可兼容大部分编译器，最好使用C99以上的标准编译器。

移植到其它运行平台，需要实现clock_arch.c文件中的clock_time()函数，它返回当前系统的时刻，这将作为协程调度的时间基准。

该框架是基于非抢占式模式进行设计的，如果在中断中使用信号量或任务创建，需要自行做保护处理，这里不做所有接口中断安全的保证。

在大部分情况下，协程内禁止使用局部变量，所有的协程函数体中变量必须声明为静态的。除非该变量的生命周期极短，且它的生命周期内没有发生协程调度。

对于低功耗模式来说，使用存在一些限制。当使用PT_WAIT_UNTIL()、PT_WAIT_WHILE()、PT_SEM_WAIT()接口时，协程将进入无超时的等待状态，此时系统无法评估全局的低功耗等待时间，因此，当使用这些接口时，将不能使用PT_TASK_IDLE_TIME()去评估休眠的时长。

## 所有的接口
| 函数名                                     | 功能        |
|-------------------------------------------|-----------|
| PT_TASK_SCHEDULE()                        | 协程调度      |
| PT_TASK_IDLE_TIME()                       | 获取协程调度可休眠的时长 |
| PT_TASK_NUMS()                            | 获取当前协程数量  |
| PT_TASK_RUN(func)                         | 创建一个协程    |
| PT_THREAD_DECL(name, body)                | 定义协程函数体   |
| PT_TASK_DELAY(pt, ms)                     | 延时函数      |
| PT_INVOK(body)                            | 协程异步执行    |
| PT_YIELD(pt)                              | 协程主动让出执行权 |
| PT_EXIT(pt)                               | 协程主动退出    |
| PT_WAIT_UNTIL(pt, condition)              | 等待条件成立    |
| PT_WAIT_WHILE(pt, cond)                   | 当条件成立时等待  |
| clock_time()                              | 获取当前系统时钟  |
| PT_SEM_INIT(sem, count)                   | 初始化信号量    |
| PT_SEM_WAIT(pt, sem)                      | 等待信号量     |
| PT_SEM_WAIT_TIMEOUT(pt, sem, timeout, ret)| 可超时的等待信号量 |
| PT_SEM_SIGNAL(pt, sem)                    | 发出一个信号量   |


## 编译及测试
```bash
make test
```
