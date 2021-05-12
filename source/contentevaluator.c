#include "contentevaluator.h"

#include "recorder.h"
#include "shared_values.h"

#include <stdio.h>
#include <stdbool.h>

#include "cJSON.h"

#define MAX_RECORDERS_NUM 2

void cacheContent(recorderStruct* recorderRef, char* content);
void cacheValidatedLine(recorderStruct* recorderRef, char* content);
void cacheLineToBeValidated(recorderStruct* recorderRef, char* content, int axisIndex, bool reversalDirection, char* op);
void evaluateCacheAndPrintFile(int id);
void clearCacheAndPrintFile(int id);

FILE* openFile(const char* name, const char* mod);
void write(int recorderStructId, char* content);
void closeFile(FILE* filePointer);

recorderStruct recorders[MAX_RECORDERS_NUM];

int recorderStackIndex = 0;

int startRecorder(char* name)
{
	recorderStruct recorder = { NULL, false };

	recorders[recorderStackIndex] = recorder;

	initRecorder(&recorders[recorderStackIndex], name);

	return recorderStackIndex++;
}

void removeRecorder(int id)
{
	for (int i = id; i < recorderStackIndex - 1; ++i)
	{
		recorders[i] = recorders[i + 1];
	}
	recorderStackIndex--;
}

void stopRecorder(int id)
{
	if (id < 0) return;
	if (id >= recorderStackIndex) return;
	clearCacheAndPrintFile(id);
	removeRecorder(id);
}

void recordContent(char* content)
{
	for (int i = 0; i < recorderStackIndex; ++i)
	{
		cacheContent(&recorders[i], content);
		if (recorders[i].cacheIndex >= (recorders[i].cacheSize))
		{
			evaluateCacheAndPrintFile(i);
		}
	}
}

void cacheContent(recorderStruct* recorderRef, char* content)
{
	cJSON* root = cJSON_Parse(content);
	char* type = cJSON_GetObjectItem(root, "type")->valuestring;

	if (strcmp(type, "BT") == 0)
	{
		cacheValidatedLine(recorderRef, content);
	}
	else if (strcmp(type, "AX") == 0)
	{
		int axisIndex = cJSON_GetObjectItem(root, "id")->valueint;

		if (axisIndex == 5)
		{
			cacheValidatedLine(recorderRef, content);
		}
		else
		{
			int axisValue = cJSON_GetObjectItem(root, "val")->valueint;

			int axisPreviousValue = recorderRef->axisPreviousValues[axisIndex];
			int axisPreviousDirection = recorderRef->axisPreviousDirections[axisIndex];

			int axisDirection = (axisValue - (axisPreviousValue) > 0) ? 1 : -1;
			bool reversalDirection = axisDirection * (axisPreviousDirection) <= 0;

			setAxisPreviousValue(recorderRef, axisIndex, axisValue);
			setAxisPreviousDirection(recorderRef, axisIndex, axisDirection);

			char* op = cJSON_GetObjectItem(root, "op")->valuestring;

			cacheLineToBeValidated(recorderRef, content, axisIndex, reversalDirection, op);
		}
	}

	cJSON_Delete(root);
}

void cacheValidatedLine(recorderStruct* recorderRef, char* content)
{
	setCacheLine(recorderRef, recorderRef->cacheIndex, content);
	setCacheLinesToBeDeleted(recorderRef, recorderRef->cacheIndex, false);
	setCacheIndex(recorderRef, recorderRef->cacheIndex + 1);
}

void cacheLineToBeValidated(recorderStruct* recorderRef, char* content, int axisIndex, bool reversalDirection, char* op)
{
	setCacheLine(recorderRef, recorderRef->cacheIndex, content);
	setCacheLinesToBeDeleted(recorderRef, recorderRef->cacheIndex, false);

	//if previous value was not local maximum or minimum
	if (!reversalDirection)
	{
		int cacheLineToBeDeletedIndex = recorderRef->axisWaitingToBeValidatedPointers[axisIndex];
		if (cacheLineToBeDeletedIndex > 0)
		{
			setCacheLinesToBeDeleted(recorderRef, cacheLineToBeDeletedIndex, true);
		}
	}
	if (strcmp(op, "START") == 0 || strcmp(op, "STOP") == 0)
	{
		setAxisWaitingToBeValidatedPointer(recorderRef, axisIndex, -1);
	}
	if (strcmp(op, "CONTINUE") == 0)
	{
		setAxisWaitingToBeValidatedPointer(recorderRef, axisIndex, recorderRef->cacheIndex);
	}

	setCacheIndex(recorderRef, recorderRef->cacheIndex + 1);
}

void evaluateCacheAndPrintFile(int id)
{
	recorderStruct* recorderRef = &recorders[id];
	int firstUnsafeCacheIndex = recorderRef->cacheSize;
	for (int i = 0; i < AXIS_NUM; ++i)
	{
		int axisWaitingToBeValidatedPointer = recorderRef->axisWaitingToBeValidatedPointers[i];
		if ((axisWaitingToBeValidatedPointer >= 0) && (axisWaitingToBeValidatedPointer < firstUnsafeCacheIndex))
		{
			firstUnsafeCacheIndex = axisWaitingToBeValidatedPointer;
		}
	}

	char* fileNewContent = (char*)malloc((sizeof(char)) * FILE_ROW_MAX_SIZE * recorderRef->cacheSize);
	strcpy(fileNewContent, "");

	for (int i = 0; (i < firstUnsafeCacheIndex && i < recorderRef->cacheIndex); ++i)
	{
		if (!recorderRef->cacheLinesToBeDeleted[i])
		{
			strcat(fileNewContent, recorderRef->cache[i]);
			strcat(fileNewContent, "\n");
		}

		//remove the cache lines under maxSafeCacheIndex
		if (i + firstUnsafeCacheIndex < recorderRef->cacheSize)
		{
			setCacheLine(recorderRef, i, recorderRef->cache[i + firstUnsafeCacheIndex]);

			bool shoulBeDeleted = recorderRef->cacheLinesToBeDeleted[i + firstUnsafeCacheIndex];

			setCacheLinesToBeDeleted(recorderRef, i, shoulBeDeleted);
		}
	}

	write(id, fileNewContent);
	free(fileNewContent);

	//rebased cache index
	if (firstUnsafeCacheIndex < 0)
	{
		setCacheIndex(recorderRef, 0);
	}
	else
	{
		int newIndex = recorderRef->cacheIndex - firstUnsafeCacheIndex;
		setCacheIndex(recorderRef, newIndex);
	}
	
	//rebased axis pointers to cache
	for (int i = 0; i < AXIS_NUM; ++i)
	{
		if (recorderRef->axisWaitingToBeValidatedPointers[i] >= 0)
		{
			int newIndex = recorderRef->axisWaitingToBeValidatedPointers[i] - firstUnsafeCacheIndex - 1;
			setAxisWaitingToBeValidatedPointer(recorderRef, i, newIndex);
		}
	}
}

void clearCacheAndPrintFile(int id)
{
	recorderStruct* recorderRef = &recorders[id];

	char* fileNewContent = (char*)malloc((sizeof(char)) * FILE_ROW_MAX_SIZE * recorderRef->cacheSize);
	strcpy(fileNewContent, "");

	for (int i = 0; i < recorderRef->cacheIndex; ++i)
	{
		if (!recorderRef->cacheLinesToBeDeleted[i])
		{
			strcat(fileNewContent, recorderRef->cache[i]);
			strcat(fileNewContent, "\n");
		}
	}

	write(id, fileNewContent);
	free(fileNewContent);

	//rebased cache index
	setCacheIndex(recorderRef, -1);
	//rebased axis pointers to cache
	for (int i = 0; i < AXIS_NUM; ++i)
	{
		setAxisWaitingToBeValidatedPointer(recorderRef, i, -1);
	}
}


FILE* openFile(const char* name, const char* mod)
{
	return fopen(name, "a");
}

void write(int recorderStructId, char* content) {
	FILE* file = openFile(recorders[recorderStructId].fileName, "a");
	fprintf(file, content);
	closeFile(file);
}

void closeFile(FILE* filePointer)
{
	if (filePointer == NULL)
	{
		return;
	}
	fclose(filePointer);
}
