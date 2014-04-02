#ifndef _frame_h
#define _frame_h

#include "pthread.h"

class Frame {
public:
    Frame(int maxPcmSize, int maxEncSize);
    ~Frame();

    int getPcmSize();
    char* getPcmBuffer();

    int getEncSize();
    void setEncSize(int size);
    char* getEncBuffer();

    long getTimestamp();
    void setTimestamp(long ts);

private:
    int pcmSize;
    int encSize;

    int maxPcmSize;
    int maxEncSize;

    char* pcmBuffer;
    char* encBuffer;

    long timestamp;
};

class FrameBuffer {
public:
    FrameBuffer(int frameCount, int maxPcmSize, int maxEncSize);
    ~FrameBuffer();

    int getPcmSize();
    char* getPcmBuffer();

    int getFrameCount();
    Frame* getFrame(int i);
    void putFrame(int i, Frame *frame);
    FrameBuffer* clone();

private:
    int pcmSize;
    char* pcmBuffer;
    int frameCount;
    int maxPcmSize;
    int maxEncSize;
};

#endif // _frame_h
