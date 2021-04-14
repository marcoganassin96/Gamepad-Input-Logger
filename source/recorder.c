#include "recorder.h"

#include "shared_values.h"

#include <stdbool.h>
#include <stdio.h>

void initRecorder(recorderStruct* recorder, char* fileName)
{
	recorder->fileName = fileName;

	memset(recorder->axisPreviousValues, 0, sizeof(int) * AXIS_NUM);

	memset(recorder->axisPreviousDirections, 0, sizeof(int) * AXIS_NUM);

	recorder->cacheSize = CACHE_LINES_NUM;

	memset(recorder->cache, 0, sizeof(char*) * CACHE_LINES_NUM);

	memset(recorder->cacheLinesToBeDeleted, 0, sizeof(bool) * CACHE_LINES_NUM);

	memset(recorder->axisWaitingToBeValidatedPointers, -1, sizeof(int) * AXIS_NUM);

	recorder->cacheIndex = 0;
}


void setAxisPreviousValue(recorderStruct* recorderRef, int axisIndex, int value)
{
	if (axisIndex >= sizeof(recorderRef->axisPreviousValues)/sizeof(int))
	{
		return;
	}
	recorderRef->axisPreviousValues[axisIndex] = value;
}

void setAxisPreviousDirection(recorderStruct* recorderRef, int axisIndex, int direction)
{
	if (axisIndex >= sizeof(recorderRef->axisPreviousValues) / sizeof(int))
	{
		return;
	}
	recorderRef->axisPreviousDirections[axisIndex] = direction;
}


void setCacheLine(recorderStruct* recorderRef, int cacheLine, char* value)
{
	if (cacheLine >= recorderRef->cacheSize)
	{
		return;
	}
	if (recorderRef->cache[cacheLine] == NULL)
		recorderRef->cache[cacheLine] = malloc(sizeof(char)* FILE_ROW_MAX_SIZE);
	strncpy(recorderRef->cache[cacheLine], value, FILE_ROW_MAX_SIZE);
}

void setCacheIndex(recorderStruct* recorderRef, int cacheIndex)
{
	if (cacheIndex > recorderRef->cacheSize)
	{
		return;
	}
	recorderRef->cacheIndex = cacheIndex;
}


void setCacheLinesToBeDeleted(recorderStruct* recorderRef, int cacheLine, bool value)
{
	if (cacheLine >= recorderRef->cacheSize)
	{
		return;
	}
	recorderRef->cacheLinesToBeDeleted[cacheLine] = value;
}

void setAxisWaitingToBeValidatedPointer(recorderStruct* recorderRef, int axisIndex, int value) {
if (axisIndex > recorderRef->cacheSize)
	{
		return;
	}
	recorderRef->axisWaitingToBeValidatedPointers[axisIndex] = value;
}

void fill_int_array(int* array, int size, int value)
{
	for (int i = 0; i < size; ++i)
	{
		array[i] = value;
	}
}
