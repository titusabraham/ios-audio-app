/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * Qualcomm Technologies, Inc. ("Proprietary Information").  You shall
 * not disclose such Proprietary Information, and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Qualcomm Technologies, Inc.
 *
 * FILE:GenerateFrame.cpp
 *
 * DESCRIPTION:
 */
#include "GenerateFrame.h"
#include <assert.h>
//#define GENFRAME_DEBUG

GenerateFrame::GenerateFrame(int inputPCMSize,int outputPCMSize)
{
#ifdef GENFRAME_DEBUG
    printf("[MEDIA-GEN] GenerateFrame(inSize=%d outSize=%d)\n",inputPCMSize,outputPCMSize);
#endif
    nMaxInputPCMSize = inputPCMSize;
    nMaxOutputPCMSize = outputPCMSize;
    if(nMaxInputPCMSize > nMaxOutputPCMSize)
    {
        nMaxPCMBufferSize = nMaxInputPCMSize * 2;
    }
    else
    {
        nMaxPCMBufferSize = nMaxOutputPCMSize * 2;
    }
    nAvailablePCMsize = 0;
    pPCMBuffer = (char*)malloc(sizeof(char)*nMaxPCMBufferSize);
    genBuffer.isAvailable = false;
    genBuffer.GenBuffer = (char*)malloc(sizeof(char)*nMaxOutputPCMSize);
#ifdef GENFRAME_DEBUG
    printGenDetails();
#endif
}

GenerateFrame::~GenerateFrame(void)
{
#ifdef GENFRAME_DEBUG
    printf("[MEDIA-GEN] ~GenerateFrame\n");
#endif
    if(pPCMBuffer)
    {
        free(pPCMBuffer);
        pPCMBuffer = NULL;
    }
    if(genBuffer.GenBuffer)
    {
        free(genBuffer.GenBuffer);
        genBuffer.GenBuffer = NULL;
    }
}

void GenerateFrame::ResetGenData()
{
#ifdef GENFRAME_DEBUG
    printf("[MEDIA-GEN] ResetGenData\n");
#endif
    if(pPCMBuffer)
    {
        memset(pPCMBuffer,0,sizeof(char)*nMaxPCMBufferSize);
    }
    else
    {
        assert(0);
    }
    if(genBuffer.GenBuffer)
    {
        memset(genBuffer.GenBuffer,0,(sizeof(char)*nMaxOutputPCMSize));
    }
    else
    {
        assert(0);
    }
    nAvailablePCMsize = 0;
    genBuffer.isAvailable = false;
}

void GenerateFrame::ResetFrameData()
{
#ifdef GENFRAME_DEBUG
    printf("[MEDIA-GEN] ResetFrameData\n");
#endif
    if(pPCMBuffer)
    {
        memset(pPCMBuffer,0,sizeof(char)*nMaxPCMBufferSize);
    }
    else
    {
        assert(0);
    }
    nAvailablePCMsize = 0;
}

void GenerateFrame::printGenDetails()
{
    printf("[MEDIA-GEN]: nMaxInputPCMSize=%d nMaxOutputPCMSize=%d nMaxPCMBufferSize=%d nAvailablePCMsize=%d\n",nMaxInputPCMSize,nMaxOutputPCMSize,nMaxPCMBufferSize,nAvailablePCMsize);
}

GenerateFrameDataType* GenerateFrame::GetGeneratedFrame()
{
    return &genBuffer;
}

PCMGenType GenerateFrame::GeneratePCMFrame(char* inputPCMBuffer,int currentInputSize)
{
    PCMGenType retVal = PCM_FRAME_UNAVAILABLE;
    int curInSize = nMaxInputPCMSize;
    if(currentInputSize)
    {
        curInSize = currentInputSize;
    }
#ifdef GENFRAME_DEBUG
    printGenDetails();
#endif
    
    assert(genBuffer.GenBuffer!=NULL);
    genBuffer.isAvailable = false;
    *genBuffer.GenBuffer = NULL;
    do {
        //Step 1: Append input Data if not null to existing buffer
        if(inputPCMBuffer)
        {
            AppenedDataToPCMBuffer(inputPCMBuffer,curInSize);
        }
        //Step 2: Check if Available data is enough to generate output PCM
        if(nAvailablePCMsize >= nMaxOutputPCMSize)
        {
            genBuffer.isAvailable = true;
            MoveDataToPCMBuffer();
            if(nAvailablePCMsize >= nMaxOutputPCMSize)
            {
                retVal = PCM_FRAME_ACQUIRED_WITH_MORE;
            }
            else
            {
                retVal = PCM_FRAME_ACQUIRED;
            }
        }
        
    } while (0);
    return retVal;
}

void GenerateFrame::AppenedDataToPCMBuffer(char* inputPCMBuffer,int currentInputSize)
{
    assert(inputPCMBuffer!=NULL);
    char* destBuffer = &(pPCMBuffer[nAvailablePCMsize]);
    memcpy(destBuffer, inputPCMBuffer, currentInputSize);
    nAvailablePCMsize += currentInputSize;
#ifdef GENFRAME_DEBUG
    printf("[MEDIA-GEN] AppenedDataToPCMBuffer with now AvailableSize=%d\n",nAvailablePCMsize);
#endif
}

void GenerateFrame::MoveDataToPCMBuffer()
{
    assert(genBuffer.GenBuffer!=NULL);
    memcpy(genBuffer.GenBuffer, pPCMBuffer, nMaxOutputPCMSize);
    int remaining = nAvailablePCMsize - nMaxOutputPCMSize;
    if(remaining > 0)
    {
        //Copy remaining to a temp buffer
        char *tempBuffer = (char*)malloc(sizeof(char)*remaining);
        memcpy(tempBuffer, &(pPCMBuffer[nMaxOutputPCMSize]),sizeof(char)*remaining);
        ResetFrameData();
        memcpy(pPCMBuffer, tempBuffer,sizeof(char)*remaining);
        nAvailablePCMsize = remaining;
        free(tempBuffer);
    }
    else
    {
        ResetFrameData();
    }
#ifdef GENFRAME_DEBUG
    printf("[MEDIA-GEN] MoveDataToPCMBuffer with now AvailableSize=%d\n",nAvailablePCMsize);
#endif
}
