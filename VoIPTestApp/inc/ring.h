/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * Qualcomm Technologies, Inc. ("Proprietary Information").  You shall
 * not disclose such Proprietary Information, and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Qualcomm Technologies, Inc.
 *
 * FILE:ring.h
 *
 * DESCRIPTION:
 */

#ifndef iOSVocoder_ring_h
#define iOSVocoder_ring_h

#include "pthread.h"
#include <stdio.h>

//others
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "timesec.h"

//#ifdef __cplusplus
//extern "C" {
//#endif


template <class T>
class RingBuffer {
public:
    RingBuffer(int ringSize)
    {
        this->ringSize = ringSize;
        
        ring = new T*[ringSize];
        for (int i = 0; i < ringSize; i++)
        {
            ring[i] = NULL;
        }
        head = 0;
        tail = 0;
        
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&condition, NULL);
    }
    
    ~RingBuffer()
    {
        delete[] ring;
    }
    
    void clear()
    {
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < ringSize; i++)
        {
            if (ring[i])
            {
                delete ring[i];
                ring[i] = NULL;
            }
        }
        head = 0;
        tail = 0;
        //pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
    }
    
    int getSize()
    {
        return ringSize;
    }
    int getQueueSize()
    {
        if (tail >= head)
        {
            return tail - head;
        }
        else
        {
            return ringSize - head + tail; // XXX off-by-one?
        }
    }
    
    bool empty()
    {
        return head == tail;
    }
    void enqueue(T* item)
    {
        pthread_mutex_lock(&mutex);
        if (tail == head)
        {
            if (ring[head])
            {
                T* i = ring[head];
                if (i)
                {
                    // XXX discard entry
                    delete i;
                }
                head++;
                if (head == ringSize)
                {
                    head = 0;
                }
            }
        }
        ring[tail++] = item;
        if (tail == ringSize)
        {
            tail = 0;
        }
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
    }
    void enqueueBlock(T* item)
    {
        pthread_mutex_lock(&mutex);
        
        if (ring[head])
        {
            // wait for something to be dequeue'd
            while (tail == head)
            {
                pthread_cond_wait(&condition, &mutex);
            }
        }
        
        ring[tail++] = item;
        if (tail == ringSize)
        {
            tail = 0;
        }
        
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
    }
    T* peek(long waitms=0)
    {
        T* item = NULL;
        pthread_mutex_lock(&mutex);
        while (ring[head] == NULL)
        {
            if (waitms)
            {
                //struct timespec ts;
                //clock_gettime(CLOCK_REALTIME, &ts);
                //ts_add_msec(&ts, waitms);
                //pthread_cond_timedwait(&condition, &mutex, &ts);
                struct timespec ts;
                struct timeval time;
                gettimeofday(&time, NULL);
                ts.tv_sec = time.tv_sec;
                ts.tv_nsec = usec_to_nsec(time.tv_usec);
                ts_add_msec(&ts, waitms);
                pthread_cond_timedwait(&condition, &mutex, &ts);
                break;
            }
            else
            {
                pthread_cond_wait(&condition, &mutex);
            }
        }
        
        // XXX use peekNonblock
        if (ring[head])
        {
            item = ring[head];
        }
        pthread_mutex_unlock(&mutex);
        
        return item;
    }
    T* peekNonblock()
    {
        T* item = NULL;
        pthread_mutex_lock(&mutex);
        if (ring[head])
        {
            item = ring[head];
        }
        pthread_mutex_unlock(&mutex);
        return item;
    }
    T* dequeue(long waitms=0)
    {
        T* item = NULL;
        pthread_mutex_lock(&mutex);
        while (ring[head] == NULL)
        {
            if (waitms)
            {
                //struct timespec ts;
                //clock_gettime(CLOCK_REALTIME, &ts);
                //ts_add_msec(&ts, waitms);
                //pthread_cond_timedwait(&condition, &mutex, &ts);
                
                struct timespec ts;
                struct timeval time;
                gettimeofday(&time, NULL);
                ts.tv_sec = time.tv_sec;
                ts.tv_nsec = usec_to_nsec(time.tv_usec);
                ts_add_msec(&ts, waitms);
                pthread_cond_timedwait(&condition, &mutex, &ts);
                break;
            }
            else
            {
                pthread_cond_wait(&condition, &mutex);
            }
        }
        
        // XXX use dequeueNonblock
        if (ring[head])
        {
            item = ring[head];
            ring[head] = NULL;
            head++;
            if (head == ringSize)
            {
                head = 0;
            }
            pthread_cond_signal(&condition);
        }
        pthread_mutex_unlock(&mutex);
        
        return item;
    }
    T* dequeueNonblock()
    {
        T* item = NULL;
        pthread_mutex_lock(&mutex);
        if (ring[head])
        {
            item = ring[head];
            ring[head] = NULL;
            head++;
            if (head == ringSize)
            {
                head = 0;
            }
            pthread_cond_signal(&condition);
        }
        pthread_mutex_unlock(&mutex);
        return item;
    }
    
private:
    int ringSize;
    int head;
    int tail;
    T** ring;
    
    pthread_mutex_t mutex;
    pthread_cond_t condition;
};

//#ifdef __cplusplus
//}
//#endif

#endif
