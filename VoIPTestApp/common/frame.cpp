#include "frame.h"
#include <string.h>

Frame::Frame(int maxPcmSize, int maxEncSize) {
    this->maxPcmSize = maxPcmSize;
    this->maxEncSize = maxEncSize;

    pcmSize = maxPcmSize;
    encSize = maxEncSize;

    pcmBuffer = new char[maxPcmSize];
    encBuffer = new char[maxEncSize];
}

Frame::~Frame() {
    delete [] pcmBuffer;
    delete [] encBuffer;
}

int Frame::getPcmSize() {
    return pcmSize;
}

char* Frame::getPcmBuffer() {
    return pcmBuffer;
}

int Frame::getEncSize() {
    return encSize;
}

void Frame::setEncSize(int size) {
    encSize = size;
}

char* Frame::getEncBuffer() {
    return encBuffer;
}

FrameBuffer::FrameBuffer(int frameCount, int maxPcmSize, int maxEncSize) {
    this->frameCount = frameCount;
    this->maxPcmSize = maxPcmSize;
    this->maxEncSize = maxEncSize;

    pcmSize = maxPcmSize * frameCount;
    pcmBuffer = new char[pcmSize];
}

FrameBuffer::~FrameBuffer() {
    delete [] pcmBuffer;
}

FrameBuffer* FrameBuffer::clone() {
    FrameBuffer* fb = new FrameBuffer(frameCount, maxPcmSize, maxEncSize);
    memcpy(fb->pcmBuffer, pcmBuffer, pcmSize);
    return fb;
}

int FrameBuffer::getPcmSize() {
    return pcmSize;
}

char* FrameBuffer::getPcmBuffer() {
    return pcmBuffer;
}

int FrameBuffer::getFrameCount() {
    return frameCount;
}

Frame* FrameBuffer::getFrame(int i) {
    Frame* frame = new Frame(maxPcmSize, maxEncSize);
    memcpy(frame->getPcmBuffer(), pcmBuffer + (i*maxPcmSize), maxPcmSize);
    return frame;
}

void FrameBuffer::putFrame(int i, Frame *frame) {
    memcpy(pcmBuffer + (i*maxPcmSize), frame->getPcmBuffer(), maxPcmSize);
}
