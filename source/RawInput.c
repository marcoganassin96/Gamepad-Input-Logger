///////////////////////////////////////////////////////////////////////////////
//
// Raw Input API sample showing joystick support
//
// Author: Alexander Böcken
// Date:   04/22/2011
//
// Copyright 2011 Alexander Böcken
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include <Windows.h>
#include <tchar.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <hidsdi.h>
#include <time.h>
#include <omp.h>
#include <stdbool.h>

#include "contentevaluator.h"
#include "shared_values.h"

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#define WC_MAINFRAME	TEXT("MainFrame")
#define MAX_BUTTONS		128
#define CHECK(exp)		{ if(!(exp)) goto Error; }
#define SAFE_FREE(p)	{ if(p) { HeapFree(hHeap, 0, p); (p) = NULL; } }

//declarations
void initState();
void recordButtons();
void printAxis();
void printTriggers();
void printArrows();
double getSecondsFromStart();
int isAxisMoving(int index);

bool bButtonStates[MAX_BUTTONS];
bool bPreviousButtonStates[MAX_BUTTONS];

bool bAxisStates[MAX_BUTTONS];
bool bPreviousAxisStates[MAX_BUTTONS];

LONG values[MAX_BUTTONS];
LONG previousValues[MAX_BUTTONS];

LONG axis[MAX_BUTTONS];
LONG previousAxis[MAX_BUTTONS];

INT  g_NumberOfButtons;
USHORT capsLength;

bool isRecording = FALSE;
int recorderIndex;
char fileName[50];

bool wasPressingStart = false;
float startPressingTime;

bool recordingStateJustChanged = false;

clock_t clockStart;

void ParseRawInput(PRAWINPUT pRawInput)
{
	PHIDP_PREPARSED_DATA pPreparsedData;
	HIDP_CAPS            Caps;
	PHIDP_BUTTON_CAPS    pButtonCaps;
	PHIDP_VALUE_CAPS     pValueCaps;
	UINT                 bufferSize;
	HANDLE               hHeap;
	USAGE                usage[MAX_BUTTONS];
	ULONG                i, usageLength, value;

	LONG                scaledValue;

	pPreparsedData = NULL;
	pButtonCaps    = NULL;
	pValueCaps     = NULL;
	hHeap          = GetProcessHeap();

	//
	// Get the preparsed data block
	//

	CHECK( GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize) == 0 );
	CHECK( pPreparsedData = (PHIDP_PREPARSED_DATA)HeapAlloc(hHeap, 0, bufferSize) );
	CHECK( (int)GetRawInputDeviceInfo(pRawInput->header.hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &bufferSize) >= 0 );

	//
	// Get the joystick's capabilities
	//

	// Button caps
	CHECK( HidP_GetCaps(pPreparsedData, &Caps) == HIDP_STATUS_SUCCESS )
	CHECK( pButtonCaps = (PHIDP_BUTTON_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_BUTTON_CAPS) * Caps.NumberInputButtonCaps) );

	capsLength = Caps.NumberInputButtonCaps;
	CHECK( HidP_GetButtonCaps(HidP_Input, pButtonCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS )
	g_NumberOfButtons = pButtonCaps->Range.UsageMax - pButtonCaps->Range.UsageMin + 1;

	// Value caps
	CHECK( pValueCaps = (PHIDP_VALUE_CAPS)HeapAlloc(hHeap, 0, sizeof(HIDP_VALUE_CAPS) * Caps.NumberInputValueCaps) );
	capsLength = Caps.NumberInputValueCaps;
	CHECK( HidP_GetValueCaps(HidP_Input, pValueCaps, &capsLength, pPreparsedData) == HIDP_STATUS_SUCCESS )

	//
	// Get the pressed buttons
	//

	usageLength = g_NumberOfButtons;
	CHECK(
		HidP_GetUsages(
			HidP_Input, pButtonCaps->UsagePage, 0, usage, &usageLength, pPreparsedData,
			(PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
		) == HIDP_STATUS_SUCCESS );

	ZeroMemory(bButtonStates, sizeof(bButtonStates));
	for(i = 0; i < usageLength; i++)
		bButtonStates[usage[i] - pButtonCaps->Range.UsageMin] = TRUE;

	//
	// Get the state of discrete-valued-controls
	//

	for(i = 0; i < Caps.NumberInputValueCaps; i++)
	{
		CHECK(
			HidP_GetUsageValue(
				HidP_Input, pValueCaps[i].UsagePage, 0, pValueCaps[i].Range.UsageMin, &value, pPreparsedData,
				(PCHAR)pRawInput->data.hid.bRawData, pRawInput->data.hid.dwSizeHid
			) == HIDP_STATUS_SUCCESS );

		switch (i)
		{
		case 1: // left stick horizontal
		case 3:	// right stick horizontal
			values[i] = (LONG)((value/ 65535.f) * 2000) - 1000;
			break;
		case 0: // left stick vertical
		case 2: // right stick vertical
			values[i] = -((LONG)((value / 65535.f) * 2000) - 1000);
			break;
		case 4:	// triggers
			values[i] = (LONG)((value / 65535.f) * 2000) - 1000;
			break;
		case 5:	// arrows
			values[i] = value;
			break;
		default:
			values[i] = value;
			break;
		}
	}

	//
	// Clean up
	//

Error:
	SAFE_FREE(pPreparsedData);
	SAFE_FREE(pButtonCaps);
	SAFE_FREE(pValueCaps);
}

void DrawGraphical(HDC hDC)
{
	HBRUSH hOldBrush, hBr;
	TCHAR  sz[4];


	RECT   buttonRect;
	int buttonRectLeftCorner = 60;
	int buttonRectBottomCorner = 20;
	int buttonRectSize = 60;
	buttonRect.left = buttonRectLeftCorner;
	buttonRect.top = buttonRectBottomCorner;
	buttonRect.right = buttonRectLeftCorner + buttonRectSize;
	buttonRect.bottom = buttonRectBottomCorner + buttonRectSize;

	if (isRecording)
	{
		hBr = CreateSolidBrush(RGB(192, 0, 0));
		hOldBrush = (HBRUSH)SelectObject(hDC, hBr);
	}

	Ellipse(hDC, buttonRect.left, buttonRect.top, buttonRect.right, buttonRect.bottom);
	_stprintf_s(sz, ARRAY_SIZE(sz), TEXT("REC"));
	DrawText(hDC, sz, -1, &buttonRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	if (isRecording)
	{
		SelectObject(hDC, hOldBrush);
		DeleteObject(hBr);
	}

	RECT recordingInstructionsRect;
	recordingInstructionsRect.left = 60;
	recordingInstructionsRect.right = 60 + 350;
	recordingInstructionsRect.bottom = 125;
	recordingInstructionsRect.top = 125 - 30;
	TCHAR recordingInstructionsText[64];
	_stprintf_s(recordingInstructionsText, ARRAY_SIZE(recordingInstructionsText), TEXT("PRESS TOGGLE VIEW TO START/STOP RECORDING"));

	DrawText(hDC, recordingInstructionsText, -1, &recordingInstructionsRect, DT_TOP | DT_LEFT);

	RECT quittingInstructionsRect;
	quittingInstructionsRect.left = 60;
	quittingInstructionsRect.right = 60 + 300;
	quittingInstructionsRect.bottom = 160;
	quittingInstructionsRect.top = 160 - 30;
	TCHAR quittingInstructionsText[64];
	_stprintf_s(quittingInstructionsText, ARRAY_SIZE(quittingInstructionsText), TEXT("PRESS MAP (FOR 2 SECONDS) TO QUIT"));
	DrawText(hDC, quittingInstructionsText, -1, &quittingInstructionsRect, DT_TOP | DT_LEFT);
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_CREATE:
		{
			//
			// Register for joystick devices
			//

			RAWINPUTDEVICE rid[2];

			rid[0].usUsagePage = 1;
			rid[0].usUsage     = 4;	// Joystick
			rid[0].dwFlags     = RIDEV_INPUTSINK; // Recieve messages when in background.
			rid[0].hwndTarget  = hWnd;

			
			rid[1].usUsagePage = 1;
			rid[1].usUsage	   = 5;	// Gamepad - e.g. XBox 360 or XBox One controllers
			rid[1].dwFlags     = RIDEV_INPUTSINK; // Recieve messages when in background.
			rid[1].hwndTarget = hWnd;

			if(!RegisterRawInputDevices(&rid, 2, sizeof(RAWINPUTDEVICE)))
				return -1;
		}
		return 0;

	case WM_INPUT:
		{

			PRAWINPUT pRawInput;
			UINT      bufferSize;
			HANDLE    hHeap;

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));

			hHeap     = GetProcessHeap();
			pRawInput = (PRAWINPUT)HeapAlloc(hHeap, 0, bufferSize);
			if(!pRawInput)
				return 0;

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pRawInput, &bufferSize, sizeof(RAWINPUTHEADER));
			ParseRawInput(pRawInput);

			HeapFree(hHeap, 0, pRawInput);

			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
		return 0;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hWnd, &ps);
		SetBkMode(hDC, TRANSPARENT);

		DrawGraphical(hDC);

		EndPaint(hWnd, &ps);
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}


bool checkChangeRecordingState()
{
	//Toggle View button pressed for 2 seconds
	if (bButtonStates[6])
	{
		if (!recordingStateJustChanged)
		{
			recordingStateJustChanged = true;
			return true;
		}
	}
	else
	{
		recordingStateJustChanged = false;
	}
	return false;
}


bool checkCloseApplication()
{
	//Map button pressed for 2 seconds
	if (bButtonStates[7])
	{
		if (!wasPressingStart)
		{
			wasPressingStart = true;
			startPressingTime = clock()/CLOCKS_PER_SEC;
		}
		float now = clock() / CLOCKS_PER_SEC;
		float passedTime = now - startPressingTime;
		return ((clock() / CLOCKS_PER_SEC) - startPressingTime > 2);
	}
	else
	{
		wasPressingStart = false;
		return false;
	}
}


void recordInputs()
{
	recordButtons();
	for (int i = 0; i < g_NumberOfButtons; i++)
	{
		bPreviousButtonStates[i] = bButtonStates[i];
	}

	printAxis();
	printTriggers();
	printArrows();
	for (int i = 0; i < capsLength; i++)
	{
		previousAxis[i] = axis[i];
		previousValues[i] = values[i];
		bPreviousAxisStates[i] = bAxisStates[i];
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	initState();

	HWND hWnd;
	MSG msg;
	WNDCLASSEX wcex;

	wcex.cbSize        = sizeof(WNDCLASSEX);
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hInstance     = hInstance;
	wcex.lpfnWndProc   = WindowProc;
	wcex.lpszClassName = WC_MAINFRAME;
	wcex.lpszMenuName  = NULL;
	wcex.style         = CS_HREDRAW | CS_VREDRAW;

	if(!RegisterClassEx(&wcex))
		return -1;

	hWnd = CreateWindow(WC_MAINFRAME, TEXT("Joystick using Raw Input API"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	UpdateWindow(hWnd);
	ShowWindow(hWnd, nShowCmd);

	while (1)
	{
		if (checkChangeRecordingState())
		{
			if (isRecording)
			{
				stopRecorder(recorderIndex);
			}
			else
			{
				struct tm* timenow;
				time_t now = time(NULL);
				timenow = gmtime(&now);

				CreateDirectoryA("recording", NULL);
				strftime(fileName, sizeof(fileName), "recording\\recording%Y%m%d%H%M%S.txt", timenow);
				recorderIndex = startRecorder(fileName);
			}
			isRecording = !isRecording;
		}

		if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (isRecording)
		{
			recordInputs();
		}

		if (checkCloseApplication())
		{
			if (isRecording) stopRecorder(recorderIndex);
			ExitProcess(0);
		}
		UpdateWindow(hWnd);
	}
	return (int)msg.wParam;
}


double getSecondsFromStart()
{
	return (((double)clock() - clockStart)) / CLOCKS_PER_SEC;
}

void recordButtons()
{
	for (int i = 0; i < g_NumberOfButtons; i++)
	{
		double currentTime = getSecondsFromStart();
		if (bButtonStates[i] != bPreviousButtonStates[i])
		{
			char* lab;
			switch (i)
			{
				case 0: lab = "A"; break;// A
				case 1: lab = "X"; break;// B
				case 2: lab = "X"; break;// X
				case 3:	lab = "Y"; break;// Y
				case 4:	lab = "LB"; break;// LB
				case 5:	lab = "RB"; break;// RB
				case 6:	lab = "TGL_V"; break;// Toggle view
				case 7:	lab = "MAP"; break;// Map
				case 8:	lab = "L"; break;// L
				case 9:	lab = "R"; break;// R
			}

			char* op = bButtonStates[i] ? "START" : "STOP";

			char* row = (char*)malloc((128) * sizeof(char));
			sprintf(row, "{\"type\": \"BT\", \"id\": %i, \"label\": \"%s\", \"op\": \"%s\", \"time \": %.3f}", i, lab, op, currentTime);
			recordContent(row);
			free(row);
		}
	}
}

void printAxis()
{
	double currentTime = getSecondsFromStart();

	for (int i = 0; i < AXIS_NUM-1; i++)
	{
		bAxisStates[i] = (isAxisMoving(i) > 0);

		axis[i] = bAxisStates[i] ? values[i] : 0;

		if (previousAxis[i] != axis[i])
		{
			char* lab;
			switch (i)
			{
				case 0: lab = "LV"; break;// left stick vertical
				case 1: lab = "LH"; break;// left stick horizontal
				case 2: lab = "RV"; break;// right stick vertical
				case 3:	lab = "RH"; break;// right stick horizontal
			}

			char* op = "UNDEFINED";
			if (bAxisStates[i] && !bPreviousAxisStates[i]) op = "START";
			if (!bAxisStates[i] && bPreviousAxisStates[i]) op = "STOP";
			if (bAxisStates[i] && bPreviousAxisStates[i]) op = "CONTINUE";

			char* row = (char*)malloc((128) * sizeof(char));
			sprintf(row, "{\"type\": \"AX\", \"id\": %i, \"label\": \"%s\", \"op\": \"%s\", \"val\": %ld, \"time \": %.3f}", i, lab, op, axis[i], currentTime);
			recordContent(row);
			free(row);
		}
	}
}

void printTriggers()
{
	double currentTime = getSecondsFromStart();

	int triggersIndex = 4;

	axis[triggersIndex] = values[triggersIndex];
	bAxisStates[triggersIndex] = (axis[triggersIndex] != 0);

	if (previousAxis[triggersIndex] != axis[triggersIndex])
	{
		char* op = "UNDEFINED";
		if (bAxisStates[triggersIndex] && !bPreviousAxisStates[triggersIndex]) op = "START";
		if (!bAxisStates[triggersIndex] && bPreviousAxisStates[triggersIndex]) op = "STOP";
		if (bAxisStates[triggersIndex] && bPreviousAxisStates[triggersIndex]) op = "CONTINUE";

		char* row = (char*)malloc((128) * sizeof(char));
		sprintf(row, "{\"type\": \"AX\", \"id\": %i, \"label\": \"%s\", \"op\": \"%s\", \"val\": %ld, \"time \": %.3f}", triggersIndex, "TRIG", op, axis[triggersIndex], currentTime);
		recordContent(row);
		free(row);
	}
}

void printArrows()
{
	double currentTime = getSecondsFromStart();

	int arrowIndex = 5;

	axis[arrowIndex] = values[arrowIndex];
	bAxisStates[arrowIndex] = (axis[arrowIndex] > 0);

	if (previousAxis[arrowIndex] != axis[arrowIndex])
	{
		char* op;
		if (bAxisStates[arrowIndex] && !bPreviousAxisStates[arrowIndex]) op = "START";
		if (!bAxisStates[arrowIndex] && bPreviousAxisStates[arrowIndex]) op = "STOP";
		if (bAxisStates[arrowIndex] && bPreviousAxisStates[arrowIndex]) op = "CONTINUE";

		char* row = (char*)malloc((128) * sizeof(char));
		sprintf(row, "{\"type\": \"AX\", \"id\": %i, \"label\": \"%s\", \"op\": \"%s\", \"val\": %ld, \"time \": %.3f}", arrowIndex, "ARR", op, axis[arrowIndex], currentTime);
		recordContent(row);
		free(row);
	}
}

int isAxisMoving(int index)
{
	int deadZone = 100;
	//int minStreaksNumber = 20;
	int pairIndex = (index % 2 == 0) ? index + 1 : index - 1;

	int result =
		   (abs(values[index]) < deadZone)
		//&& (unchangedValuesStreaks[index] >= minStreaksNumber)
		&& (abs(values[pairIndex]) < deadZone)
		//&& (unchangedValuesStreaks[pairIndex] >= minStreaksNumber)
		? 0 : 1;

	return result;
}

void initState()
{	
	for (int i = 0; i < g_NumberOfButtons; i++)
	{
		bPreviousButtonStates[i] = FALSE;
	}

	for (int i = 0; i < capsLength; i++)
	{
		previousAxis[i] = 0;
		previousValues[i] = 0;
		bPreviousAxisStates[i] = FALSE;
	}
}
