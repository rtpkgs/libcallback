// The MIT License (MIT)
// Copyright (c) 2018 mizuka

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#include "libcallback.h" 

#define DBG_SECTION_NAME "libcallback"
#define DBG_ENABLE 
#define DBG_LEVEL        DBG_WARNING
#define DBG_COLOR
#include <rtdbg.h>

#define STATE_PORTECT_INIT  uint32_t level
#define STATE_PORTECT_START do {level = rt_hw_interrupt_disable();} while (0)
#define STATE_PORTECT_END   do {rt_hw_interrupt_enable(level);} while (0)

#define CALLBACK_LOCK(q)    rt_mutex_take(&((q)->lock), RT_WAITING_FOREVER) 
#define CALLBACK_UNLOCK(q)  rt_mutex_release(&(q->lock))

#define CALLBACK_ACK(event) do {if (&((event)->ack)) rt_completion_done(&((event)->ack));} while (0) 

struct rt_callback_event; 
typedef struct rt_callback_event *rt_callback_event_t; 

/* callback queue */ 
struct rt_callback_queue
{
    rt_list_t list;
    rt_callback_event_t current_event; 

    rt_thread_t thread; 
    const char *name; 

    struct rt_mutex lock;
};

/* callback event */ 
struct rt_callback_event
{
    rt_list_t list;
    int mode; 

    struct rt_completion ack;

    rt_err_t (*callback)(rt_callback_queue_t queue, void *p);
    void *p;

    rt_uint8_t max_num[20]; 
};

static void _callback(void *p); 
static rt_callback_event_t rt_callback_event_create(rt_err_t (*callback)(rt_callback_queue_t queue, void *p), void *p, int mode); 
static rt_err_t rt_callback_event_delete(rt_callback_event_t event); 

static void _callback(void *p)
{
    STATE_PORTECT_INIT; 
    rt_callback_queue_t queue = RT_NULL; 
    rt_callback_event_t event = RT_NULL; 

    queue = (struct rt_callback_queue *)p; 
    if(queue == RT_NULL)
    {
        LOG_E("callback queue is null, %s thread run failed.", rt_thread_self()->name); 
        goto _failed; 
    }

    while(1)
    {
        if(rt_list_isempty(&(queue->list)))
        {
            LOG_D("callback function is done, suspend %s thread.", rt_thread_self()->name); 
            rt_thread_suspend(rt_thread_self());
            rt_schedule();
        }

        STATE_PORTECT_START; 
        {
            event = rt_list_entry(queue->list.next, struct rt_callback_event, list); 
            rt_list_remove(&(event->list)); 
            queue->current_event = event;
            LOG_D("event mem addr 0x%.8x.", (int)event); 
        }
        STATE_PORTECT_END; 

        if(event->callback != RT_NULL)
        {
            event->callback(queue, event->p); 
        }

        STATE_PORTECT_START; 
        {
            queue->current_event = RT_NULL;
        }
        STATE_PORTECT_END; 

        if(event->mode == RT_CALLBACK_MODE_SYNC)
        {
            CALLBACK_ACK(event); 
        }
        
        rt_callback_event_delete(event); 
        event = RT_NULL;
    }

_failed: 
    LOG_E("callback thread %s quit run.", rt_thread_self()->name); 
    return; 
}

rt_callback_queue_t rt_callback_queue_create(const char *name, rt_uint32_t stack_size, rt_uint8_t priority)
{
    STATE_PORTECT_INIT; 
    rt_callback_queue_t queue = RT_NULL; 
    
    RT_ASSERT(name != RT_NULL); 

    queue = (rt_callback_queue_t)rt_malloc(sizeof(struct rt_callback_queue)); 
    if(queue == RT_NULL)
    {
        LOG_E("callback queue handle malloc failed."); 
        goto _failed; 
    }
    else
    {
        rt_memset(queue, 0x00, sizeof(struct rt_callback_queue)); 
    }

    STATE_PORTECT_START; 
    {
        rt_list_init(&(queue->list)); 
        queue->current_event = RT_NULL;
        queue->thread = RT_NULL; 
        queue->name = name; 
        rt_mutex_init(&(queue->lock), name, RT_IPC_FLAG_FIFO);
    }
    STATE_PORTECT_END; 
    
    queue->thread = rt_thread_create(name, _callback, queue, stack_size, priority, 10); 
    if(queue->thread == RT_NULL)
    {
        LOG_E("callback queue thread create failed."); 
        goto _failed; 
    }
    rt_thread_startup(queue->thread);

    return queue; 

_failed: 
    if(queue->thread != RT_NULL)
    {
        rt_thread_delete(queue->thread); 
        queue->thread = RT_NULL; 
    }

    if(queue != RT_NULL)
    {
        rt_free(queue); 
        queue = RT_NULL; 
    }

    return RT_NULL; 
}

rt_err_t rt_callback_queue_delete(rt_callback_queue_t queue) 
{
    rt_err_t ret = RT_EOK; 
    rt_uint8_t stat = RT_THREAD_CLOSE;

    if(queue == RT_NULL)
    {
        ret = (-RT_EINVAL); 
        LOG_E("callback queue is null, unable to delete."); 
        goto _failed; 
    }

    while(1)
    {
        stat = queue->thread->stat & RT_THREAD_STAT_MASK; 

        if(stat == RT_THREAD_SUSPEND || stat == RT_THREAD_CLOSE)
        {
            break; 
        }
        rt_thread_mdelay(1);
    }

    if(queue->thread != RT_NULL)
    {
        rt_thread_delete(queue->thread); 
        queue->thread = RT_NULL; 
    }

    if(queue != RT_NULL)
    {
        rt_free(queue); 
        queue = RT_NULL; 
    }

    return RT_EOK; 

_failed: 
    return ret; 
}

static rt_callback_event_t rt_callback_event_create(rt_err_t (*callback)(rt_callback_queue_t queue, void *p), void *p, int mode)
{
    STATE_PORTECT_INIT; 
    rt_callback_event_t event = RT_NULL; 

    if(callback == RT_NULL)
    {
        LOG_E("callback event func is null, unable to create."); 
        goto _failed; 
    }

    if(mode >= RT_CALLBACK_MODE_NONE)
    {
        LOG_E("callback mode is illegal, unable to call."); 
        goto _failed; 
    }

    event = (rt_callback_event_t)rt_malloc(sizeof(struct rt_callback_event)); 
    if(event == RT_NULL)
    {
        LOG_E("callback event handle malloc failed."); 
        goto _failed; 
    }

    STATE_PORTECT_START; 
    {
        rt_list_init(&(event->list)); 
        event->p = p; 
        event->callback = callback; 
        event->mode = mode; 
    }
    STATE_PORTECT_END;

    if(event->mode == RT_CALLBACK_MODE_SYNC)
    {
        rt_completion_init(&(event->ack)); 
    }

    return event; 

_failed: 
    if(event != RT_NULL)
    {
        rt_free(event); 
        event = RT_NULL; 
    }

    return RT_NULL; 
}

static rt_err_t rt_callback_event_delete(rt_callback_event_t event)
{
    rt_err_t ret = RT_EOK; 

    if(event == RT_NULL)
    {
        LOG_E("callback event handle is null, unable to delete."); 
        goto _failed; 
    }

    event->p = RT_NULL; 
    event->callback = RT_NULL; 
    event->mode = RT_CALLBACK_MODE_NONE; 

    rt_free(event); 
    event = RT_NULL; 

    return RT_EOK; 

_failed: 
    return ret; 
}

rt_err_t rt_callback_call(rt_callback_queue_t queue, rt_err_t (*callback)(rt_callback_queue_t queue, void *p), void *p, int mode)
{
    STATE_PORTECT_INIT; 
    rt_err_t ret = RT_EOK; 
    rt_callback_event_t event = RT_NULL; 

    if(queue == RT_NULL)
    {
        ret = (-RT_EINVAL); 
        LOG_E("callback queue is null, unable to call."); 
        goto _failed; 
    }

    if(callback == RT_NULL)
    {
        ret = (-RT_EINVAL); 
        LOG_E("callback func is null, unable to call."); 
        goto _failed; 
    }

    CALLBACK_LOCK(queue); 

    if(mode == RT_CALLBACK_MODE_FUNC)
    {
        ret = callback(queue, event->p); 
    }
    else if((mode == RT_CALLBACK_MODE_SYNC) || (mode == RT_CALLBACK_MODE_ASYN))
    {
        event = rt_callback_event_create(callback, p, mode);
        if(event == RT_NULL)
        {
            ret = (-RT_ERROR); 
            LOG_E("callback event malloc failed, unable to call."); 
            goto _failed; 
        }

        STATE_PORTECT_START; 
        {
            rt_list_insert_after(queue->list.prev, &(event->list)); 
        }
        STATE_PORTECT_END;
        
        rt_thread_resume(queue->thread);
        rt_schedule();

        if(mode == RT_CALLBACK_MODE_SYNC)
        {
            rt_completion_wait(&(event->ack), RT_WAITING_FOREVER); 
        }
    }

    CALLBACK_UNLOCK(queue); 

    return ret; 

_failed: 
    if(event != RT_NULL)
    {
        rt_callback_event_delete(event); 
    }

    CALLBACK_UNLOCK(queue); 

    return ret; 
}

const char *rt_callback_find(rt_callback_queue_t queue)
{
    return queue->name; 
}
