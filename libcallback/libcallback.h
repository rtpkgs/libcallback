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

#ifndef __LIB_CALLBACK_H__
#define __LIB_CALLBACK_H__

#include "rtthread.h"
#include "rtdevice.h"
#include "rthw.h"

#ifndef RT_USING_HEAP
#error "ERROR: dynamic memory management must be turned on!"
#endif 

typedef enum 
{
    RT_CALLBACK_MODE_FUNC = 0, 
    RT_CALLBACK_MODE_SYNC = 1, 
    RT_CALLBACK_MODE_ASYN = 2, 
    RT_CALLBACK_MODE_NONE = 3,
}rt_callback_event_mode_t; 

struct rt_callback_queue; 
typedef struct rt_callback_queue *rt_callback_queue_t; 

rt_callback_queue_t rt_callback_queue_create(const char *name, rt_uint32_t stack_size, rt_uint8_t priority); 
rt_err_t rt_callback_queue_delete(rt_callback_queue_t queue); 

rt_err_t rt_callback_call(rt_callback_queue_t queue, rt_err_t (*callback)(rt_callback_queue_t queue, void *p), void *p, int mode); 

const char *rt_callback_find(rt_callback_queue_t queue); 

#endif
