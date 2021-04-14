#pragma once

#ifndef _RECORD_STRUCT_H_
#define _RECORD_STRUCT_H_

#include <stdbool.h>
#include <stdio.h>

#include"shared_values.h"

typedef struct
{
	char* fileName;

	int axisPreviousValues[AXIS_NUM];
	int axisPreviousDirections[AXIS_NUM];

	int cacheSize;
	char* cache[CACHE_LINES_NUM];
	int cacheIndex;

	bool cacheLinesToBeDeleted[CACHE_LINES_NUM];
	int axisWaitingToBeValidatedPointers[AXIS_NUM];
} recorderStruct;

extern void initRecorder(recorderStruct* recorder, char* fileName);


extern void setAxisPreviousValue(recorderStruct* recorderRef, int axisIndex, int value);

extern void setAxisPreviousDirection(recorderStruct* recorderRef, int axisIndex, int direction);


extern void setCacheLine(recorderStruct* recorderRef, int cacheLine, char* value);

extern void setCacheIndex(recorderStruct* recorderRef, int cacheIndex);


extern void setCacheLinesToBeDeleted(recorderStruct* recorderRef, int cacheLine, bool value);

extern void setAxisWaitingToBeValidatedPointer(recorderStruct* recorderRef, int axisIndex, int value);

#endif
