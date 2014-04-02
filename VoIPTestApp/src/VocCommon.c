/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * Qualcomm Technologies, Inc. ("Proprietary Information").  You shall
 * not disclose such Proprietary Information, and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Qualcomm Technologies, Inc.
 *
 * FILE:VocCommon.c
 *
 * DESCRIPTION: Vocoder Common Functionalities and Defines
 */
#include "VocCommon.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
// this define is used to print hex buffer for encryption verification pupose. This define and the function Trace()
// can be removed later completely if we don't need to debug encryption later on
void TraceBuffer(const char * title, const uint8* pInData, uint16 datalength)
{
    char buffer[HEX_DATA_BUFFER_LENGTH_MAX * 2 + 1];
    memset(buffer, 0, HEX_DATA_BUFFER_LENGTH_MAX * 2 + 1);

    uint16 leng = datalength > HEX_DATA_BUFFER_LENGTH_MAX ? HEX_DATA_BUFFER_LENGTH_MAX : datalength;

    uint16 i = 0;
    uint16 j = 0;
    for(i = 0; i < leng; i++)
    {
        char ch = pInData[i];
        char ch0 = (ch >> 4) & 0x0F;
        char ch1 = ch & 0x0F;
        if((0 <= ch0) && (ch0 <= 9))
        {
            buffer[j] = '0' + ch0;
        }
        else
        {
            buffer[j] = 'A' + (ch0 - 10);
        }
        j++;
        if((0 <= ch1) && (ch1 <= 9))
        {
            buffer[j] = '0' + ch1;
        }
        else
        {
            buffer[j] = 'A' + (ch1 - 10);
        }
        j++;
    }
    buffer[j] = '\0';
    if (datalength > HEX_DATA_BUFFER_LENGTH_MAX)
    {
        printf( "datalength > 256, not all data printed out\n");
    }
    printf( "ProcessPacket %s (length = %d): 0X%s \n", title, datalength, buffer);
}

