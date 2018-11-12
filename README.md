# libcallback

libcallback 是基于 rt-thread 使用的同步异步回调队列实现库. 

## 特性
1. 直接回调
2. 同步回调
3. 异步回调
4. 执行顺序确定
5. 支持性能评估

## 效果

![](https://i.imgur.com/FWB7Ywp.gif)

效果代码: 

```
#include <rtthread.h>
#include "libcallback.h" 
#include "msh.h" 

rt_err_t cb(rt_callback_queue_t queue, void *p)
{
    static int cnt = 1; 
    rt_thread_mdelay(1000); 
    rt_kprintf("cb run cnt = %d.\n", cnt++); 
    return cnt; 
}

rt_callback_queue_t queue; 

int callback_msh(void)
{
    queue = rt_callback_queue_create("cb_queue1", 1024, 22); 
    
    rt_kprintf("11111.\n"); 
    rt_callback_call(queue, cb, RT_NULL, RT_CALLBACK_MODE_ASYN);
    
    rt_kprintf("22222.\n"); 
    rt_callback_call(queue, cb, RT_NULL, RT_CALLBACK_MODE_ASYN);
    
    rt_kprintf("33333.\n"); 
    rt_callback_call(queue, cb, RT_NULL, RT_CALLBACK_MODE_ASYN);
    
    rt_kprintf("44444.\n"); 
    rt_callback_call(queue, cb, RT_NULL, RT_CALLBACK_MODE_SYNC);
    
    rt_kprintf("55555.\n"); 
    rt_callback_call(queue, cb, RT_NULL, RT_CALLBACK_MODE_FUNC);
    
    rt_kprintf("66666.\n"); 
    rt_callback_queue_delete(queue);  
    
    rt_kprintf("77777.\n"); 
    
    return RT_EOK; 
}
MSH_CMD_EXPORT_ALIAS(callback_msh, test, callback_msh); 
```

## 直接回调

1. 调用者与回调执行为同一线程
2. 回调执行完毕返回后, 调用者继续运行

```
#include "libcallback.h" 

rt_err_t cb(rt_callback_queue_t queue, void *p)
{
    rt_thread_mdelay(1000); 
    rt_kprintf("hello libcallback: %s.\n", rt_thread_self()->name); 
    return RT_EOK; 
}

/* main thread */ 
int main(void)
{
	rt_callback_queue_t queue; 
	
	queue = rt_callback_queue_create("cb_queue", 1024, 22);

	rt_kprintf("rt_callback_call start.\n"); 
	rt_callback_call(queue, cb, RT_NULL, RT_CALLBACK_MODE_FUNC);
	rt_kprintf("rt_callback_call end.\n"); 

	rt_callback_queue_delete(queue); 

	return RT_EOK;
}
```

执行结果: 

```
rt_callback_call start.
hello libcallback: main.
rt_callback_call end.
```

## 同步回调

1. 调用者与回调执行非同一线程
2. 回调执行完毕返回后, 调用者继续运行

```
#include "libcallback.h" 

rt_err_t cb(rt_callback_queue_t queue, void *p)
{
	rt_thread_mdelay(1000); 
    rt_kprintf("hello libcallback: %s.\n", rt_thread_self()->name); 
    return RT_EOK; 
}

/* main thread */ 
int main(void)
{
	rt_callback_queue_t queue; 
	
	queue = rt_callback_queue_create("cb_queue", 1024, 22);

	rt_kprintf("rt_callback_call start.\n"); 
	rt_callback_call(queue, cb, RT_NULL, RT_CALLBACK_MODE_FUNC);
	rt_kprintf("rt_callback_call end.\n"); 

	rt_callback_queue_delete(queue); 

	return RT_EOK;
}
```

执行结果: 

```
rt_callback_call start.
hello libcallback: cb_queue.
rt_callback_call end.
```

## 异步回调

1. 调用者与回调执行非同一线程
2. 调用者调用后, 立刻继续运行

```
#include "libcallback.h" 

rt_err_t cb(rt_callback_queue_t queue, void *p)
{
	rt_thread_mdelay(1000); 
    rt_kprintf("hello libcallback: %s.\n", rt_thread_self()->name); 
    return RT_EOK; 
}

/* main thread */ 
int main(void)
{
	rt_callback_queue_t queue; 
	
	queue = rt_callback_queue_create("cb_queue", 1024, 22);

	rt_kprintf("rt_callback_call start.\n"); 
	rt_callback_call(queue, cb, RT_NULL, RT_CALLBACK_MODE_ASYN);
	rt_kprintf("rt_callback_call end.\n"); 

	rt_callback_queue_delete(queue); 

	return RT_EOK;
}
```

执行结果: 

```
rt_callback_call start.
rt_callback_call end.
hello libcallback: cb_queue.
```

## 执行顺序

对于同步、异步回调, 且是同一回调队列中执行的回调都是顺序确定的，时间上先调用先执行. 

## 性能评估

1. 支持dump历史最大回调数
2. 支持dumo当前未执行的回调数量

## 许可声明

The MIT License (MIT)
Copyright (c) 2018 mizukaaaaa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.



