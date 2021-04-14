#pragma once
#ifndef _CONTENT_EVALUATOR_H_
#define _CONTENT_EVALUATOR_H_

#include <stdbool.h>
#include <stdio.h>

extern int startRecorder(char* name);

extern void recordContent(char* content);

extern void stopRecorder(int id);

#endif
