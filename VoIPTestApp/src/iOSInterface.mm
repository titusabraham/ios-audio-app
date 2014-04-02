/*
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * Qualcomm Technologies, Inc. ("Proprietary Information").  You shall
 * not disclose such Proprietary Information, and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Qualcomm Technologies, Inc.
 *
 * FILE:iOSInterface.mm
 *
 * DESCRIPTION:
 */

#include "iOSInterface.h"



void GetiOSFileName(char* str,const char* filename)
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *docs_dir = [paths objectAtIndex:0];
    const char *stringAsChar = [docs_dir cStringUsingEncoding:[NSString defaultCStringEncoding]];
    strcpy(str,stringAsChar);
    strcat(str,"/");
    strcat(str,filename);
}