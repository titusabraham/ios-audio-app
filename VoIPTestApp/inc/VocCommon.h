/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * Qualcomm Technologies, Inc. ("Proprietary Information").  You shall
 * not disclose such Proprietary Information, and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Qualcomm Technologies, Inc.
 *
 * FILE:VocCommon.h
 *
 * DESCRIPTION: Vocoder Common Functionalities and Defines
 */
#ifndef iOSVocoder_VocCommon_h
#define iOSVocoder_VocCommon_h

#include <stdio.h>
#include <stdlib.h>
#import <string.h>

#ifdef IOS_VOC_VERSION
#define DISP_VOC_VERSION IOS_VOC_VERSION
#else
#define DISP_VOC_VERSION "<internal>"
#endif

#ifdef IOS_VOC_PRODUCT_NAME
#define DISP_VOC_PRODUCT_NAME IOS_VOC_PRODUCT_NAME
#else
#define DISP_VOC_PRODUCT_NAME "VOCODER-IOS"
#endif

#define HEX_DATA_BUFFER_LENGTH_MAX 256

//Below are just used for debugging only
//#define USE_FILE_IO
//#define ENABLE_LOOPBACK
//#define ENABLE_DEBUG_LOGGING

#ifdef __cplusplus
extern "C" {
#endif

#define mvs_pkt_status_type int
#define mvs_amr_frame_type int
    
#define MVS_VOC_RATE_UNDEF 0
    
    typedef enum
    {
        VOC_RET_SUCCESS                   = 0,   // success
        VOC_RET_FAIL                      = 1,   // failure case if no other failure code matches
        VOC_RET_ERR_NO_MEMORY             = 2,   // no memory
        VOC_RET_ERR_BAD_PARAMETER         = 3,   // argument parameter cannot pass the sanity check
        VOC_RET_CODE_MAX_VALUE            = 4
    } Voc_RetValEnumType;
    
    
#ifndef _UINT32_DEFINED
    typedef unsigned long int uint32;      // Unsigned 32 bit value
#define _UINT32_DEFINED
#endif
    
#ifndef _UINT16_DEFINED
    typedef unsigned short uint16;      // Unsigned 16 bit value
#define _UINT16_DEFINED
#endif
    
#ifndef _UINT8_DEFINED
    typedef unsigned char uint8;       // Unsigned 8  bit value
#define _UINT8_DEFINED
#endif
    
#ifndef _INT32_DEFINED
    typedef signed long int int32;       // Signed 32 bit value
#define _INT32_DEFINED
#endif
    
#ifndef _INT16_DEFINED
    typedef signed short int16;       // Signed 16 bit value
#define _INT16_DEFINED
#endif
    
#ifndef _INT8_DEFINED
    typedef signed char int8;        // Signed 8  bit value
#define _INT8_DEFINED
#endif
    
    typedef struct
    {
        int rate;
    } mvs_pcm_frame_info_type;
    
    enum frames_types
    {
        FRAME_TYPE_PCM,
        FRAME_TYPE_MAX
    };
    
    typedef struct
    {
        //    mvs_frame_info_header_type hdr; /** <--- frame info header ---> **/
        //    mvs_voc_frame_info_type voc_rate;
        //    mvs_amr_frame_info_type amr_rate;
        mvs_pcm_frame_info_type pcm_rate;
        uint32 length;
    } mvs_frame_info_type;
    
void TraceBuffer(const char * title, const uint8* pInData, uint16 datalength);

#ifdef __cplusplus
}
#endif

#endif
