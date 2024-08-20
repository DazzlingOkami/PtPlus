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

#include "pt_plus.h"

LIST_HEAD(pt_pool);

void pt_task_schedule(void)
{
    int overall_state;
    int state;
    do
    {
        pt_item_t *pt_item;
        overall_state = 0;
        list_for_each_entry(&pt_pool, pt_item, pt_item_t, list)
        {
            state = pt_item->task(&(pt_item->pt));
            overall_state |= state;
            if (state >= PT_EXITED)
            {
                list_delete(&(pt_item->list));
                break;
            }
        }
        /* 
         * All coroutines need to return to PTVNet state before scheduling
         * can be ended. This is to enable timed processing of related 
         * transactions between coroutines, such as semaphore mechanisms, 
         * coroutine creation, etc.
         */
    } while (overall_state != PT_WAITING);
}

#if defined(PT_PLUS_DELAY_SUPPORT) && (PT_PLUS_DELAY_SUPPORT == 1)
clock_time_t pt_task_idle_time(void){
    clock_time_t min_idle_time;
    clock_time_t idle_time;
    pt_item_t *pt_item;
    min_idle_time = 1000;
    list_for_each_entry(&pt_pool, pt_item, pt_item_t, list)
    {
        idle_time = timer_remaining(&(pt_item->periodic));
        if(idle_time < min_idle_time)
        {
            min_idle_time = idle_time;
        }
    }
    return min_idle_time;
}
#endif
