#include <kernel.h>
#include <delaythread.h>
#include <gsKit.h>
#include <libpad.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <stdio.h>
#include <math.h>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "misc/KeyCodes.h"
#include "widget/WidgetManager.h"
#include "Ps2PadState.h"
#include "Ps2IoLock.h"

using namespace Sexy;

extern GSGLOBAL* gsGlobal;

static unsigned char sPadBuf[256] __attribute__((aligned(64)));
static bool sPadReady = false;
static unsigned short sPrevHeld = 0;
static float sCursorX = 320.0f;
static float sCursorY = 224.0f;

// Read by GLInterface::Redraw to overlay the software cursor each frame.
void Ps2GetCursorPos(float& x, float& y)
{
	x = sCursorX;
	y = sCursorY;
}

static unsigned char sLxCenter = 128;
static unsigned char sLyCenter = 128;
static unsigned char sRxCenter = 128;
static unsigned char sRyCenter = 128;
static int sCenterSamples = 0;
static int sLxSum = 0;
static int sLySum = 0;
static int sRxSum = 0;
static int sRySum = 0;
static bool sCentered = false;

static void WaitPadStable()
{
	for (int i = 0; i < 500; ++i)
	{
		if (padGetState(0, 0) == PAD_STATE_STABLE)
			return;
		DelayThread(1000);
	}
}

static void WaitPadRequest()
{
	for (int i = 0; i < 500; ++i)
	{
		int state = padGetReqState(0, 0);
		if (state == PAD_RSTAT_COMPLETE || state == PAD_RSTAT_FAILED)
			return;
		DelayThread(1000);
	}
}

static void InitPad()
{
	static bool sTriedInit = false;
	if (sTriedInit)
		return;
	sTriedInit = true;

	printf("[PS2] pad init begin\n");
	SifInitRpc(0);
	SifLoadFileInit();
	int sio2 = SifLoadModule("rom0:SIO2MAN", 0, NULL);
	int padman = SifLoadModule("rom0:PADMAN", 0, NULL);
	printf("[PS2] pad modules: SIO2MAN=%d PADMAN=%d\n", sio2, padman);

	if (padInit(0) != 1 || padPortOpen(0, 0, sPadBuf) == 0)
	{
		printf("[PS2] pad init failed\n");
		return;
	}

	WaitPadStable();
	for (int i = 0; i < 30; ++i)
	{
		padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
		WaitPadRequest();
		WaitPadStable();
		if (padInfoMode(0, 0, PAD_MODECURID, 0) == PAD_TYPE_DUALSHOCK)
			break;
		DelayThread(50000);
	}

	sPadReady = true;
	printf("[PS2] pad ready\n");
}

static bool AxisNearCenter(unsigned char v)
{
	return v >= 96 && v <= 160;
}

static void UpdateCalibration(const padButtonStatus& pad)
{
	if (sCentered)
		return;
	if (!AxisNearCenter(pad.ljoy_h) || !AxisNearCenter(pad.ljoy_v))
		return;

	sLxSum += pad.ljoy_h;
	sLySum += pad.ljoy_v;
	sRxSum += AxisNearCenter(pad.rjoy_h) ? pad.rjoy_h : 128;
	sRySum += AxisNearCenter(pad.rjoy_v) ? pad.rjoy_v : 128;
	if (++sCenterSamples < 16)
		return;

	sLxCenter = (unsigned char)(sLxSum / sCenterSamples);
	sLyCenter = (unsigned char)(sLySum / sCenterSamples);
	sRxCenter = (unsigned char)(sRxSum / sCenterSamples);
	sRyCenter = (unsigned char)(sRySum / sCenterSamples);
	sCentered = true;
}

static float Axis(unsigned char value, unsigned char center, float deadzone)
{
	float v = ((int)value - (int)center) / 127.0f;
	if (v > -deadzone && v < deadzone)
		return 0.0f;
	if (v < -1.0f)
		return -1.0f;
	if (v > 1.0f)
		return 1.0f;
	return v;
}

static void SendMouseMove(SexyAppBase* app, int x, int y)
{
	if (!app->mMouseIn)
		app->mMouseIn = true;

	int rx = x;
	int ry = y;
	app->mWidgetManager->RemapMouse(rx, ry);
	app->mLastUserInputTick = app->mLastTimerTime;
	app->mWidgetManager->MouseMove(rx, ry);
}

static void SendMouseButton(SexyAppBase* app, int x, int y, int button, bool down)
{
	int rx = x;
	int ry = y;
	app->mWidgetManager->RemapMouse(rx, ry);
	app->mLastUserInputTick = app->mLastTimerTime;
	app->mWidgetManager->MouseMove(rx, ry);
	if (down)
		app->mWidgetManager->MouseDown(rx, ry, button);
	else
		app->mWidgetManager->MouseUp(rx, ry, button);
}

static void SendKey(SexyAppBase* app, KeyCode key, bool down)
{
	app->mLastUserInputTick = app->mLastTimerTime;
	if (down)
		app->mWidgetManager->KeyDown(key);
	else
		app->mWidgetManager->KeyUp(key);
}

static void SendKeyTransitions(SexyAppBase* app, unsigned short pressed, unsigned short released)
{
	struct Mapping { unsigned short button; KeyCode key; };
	static const Mapping kMappings[] = {
		{ PS2_PAD_UP, KEYCODE_UP },
		{ PS2_PAD_DOWN, KEYCODE_DOWN },
		{ PS2_PAD_LEFT, KEYCODE_LEFT },
		{ PS2_PAD_RIGHT, KEYCODE_RIGHT },
		{ PS2_PAD_CIRCLE, KEYCODE_ESCAPE },
		{ PS2_PAD_TRIANGLE, KEYCODE_ESCAPE },
		{ PS2_PAD_START, KEYCODE_RETURN },
		{ PS2_PAD_SELECT, KEYCODE_TAB }
	};

	for (unsigned i = 0; i < sizeof(kMappings) / sizeof(kMappings[0]); ++i)
	{
		if (pressed & kMappings[i].button)
			SendKey(app, kMappings[i].key, true);
		if (released & kMappings[i].button)
			SendKey(app, kMappings[i].key, false);
	}
}

void SexyAppBase::InitInput()
{
	InitPad();
}

bool SexyAppBase::StartTextInput(std::string&)
{
	return false;
}

void SexyAppBase::StopTextInput()
{
}

bool SexyAppBase::ProcessDeferredMessages(bool)
{
	if (!mWidgetManager)
		return false;

	// All libpad calls below issue SIF RPC (EE<->IOP), which is not thread-safe.
	// Serialize against the resource-loading thread's file I/O with the global
	// SIF RPC lock; hold it only around the actual pad polling, not the widget
	// dispatch that follows.
	Ps2IoLockAcquire();
	InitPad();
	if (!sPadReady)
	{
		Ps2IoLockRelease();
		return false;
	}

	int state = padGetState(0, 0);
	if (state == PAD_STATE_DISCONN || state == PAD_STATE_ERROR)
	{
		Ps2IoLockRelease();
		sPrevHeld = 0;
		ps2PadDisconnect(0);
		return false;
	}

	padButtonStatus pad = {};
	bool gotPad = padRead(0, 0, &pad) != 0;
	Ps2IoLockRelease();
	if (!gotPad || pad.mode == 0 || pad.btns == 0x0000)
		return false;

	UpdateCalibration(pad);

	unsigned short held = (unsigned short)(0xFFFF ^ pad.btns);
	unsigned short pressed = held & ~sPrevHeld;
	unsigned short released = sPrevHeld & ~held;
	sPrevHeld = held;

	float lx = sCentered ? Axis(pad.ljoy_h, sLxCenter, 0.18f) : 0.0f;
	float ly = sCentered ? Axis(pad.ljoy_v, sLyCenter, 0.18f) : 0.0f;
	float rx = sCentered ? Axis(pad.rjoy_h, sRxCenter, 0.18f) : 0.0f;
	float ry = sCentered ? Axis(pad.rjoy_v, sRyCenter, 0.18f) : 0.0f;

	ps2PadUpdateSnapshot(0, true, lx, ly, rx, ry, held, pressed, released);

	const float screenW = gsGlobal ? (float)gsGlobal->Width : 640.0f;
	const float screenH = gsGlobal ? (float)gsGlobal->Height : 448.0f;
	// Cursor speed in screen pixels per frame. The stick response is squared
	// (keeping sign) so small deflections give fine control and full tilt
	// still moves fast; the D-Pad uses the constant step.
	const float step = 3.0f;
	// The framebuffer has non-square pixels (e.g. 640x224 scanned out as 4:3),
	// so scale the vertical step to keep the on-screen speed uniform.
	const float stepY = step * (screenH / screenW) * (4.0f / 3.0f);
	float dx = lx * fabsf(lx) * step;
	float dy = ly * fabsf(ly) * stepY;
	if (held & PS2_PAD_LEFT)  dx -= step;
	if (held & PS2_PAD_RIGHT) dx += step;
	if (held & PS2_PAD_UP)    dy -= stepY;
	if (held & PS2_PAD_DOWN)  dy += stepY;

	if (dx != 0.0f || dy != 0.0f)
	{
		sCursorX += dx;
		sCursorY += dy;
		if (sCursorX < 0.0f) sCursorX = 0.0f;
		if (sCursorY < 0.0f) sCursorY = 0.0f;
		if (sCursorX > screenW - 1.0f) sCursorX = screenW - 1.0f;
		if (sCursorY > screenH - 1.0f) sCursorY = screenH - 1.0f;
		SendMouseMove(this, (int)sCursorX, (int)sCursorY);
	}

	if (pressed & PS2_PAD_CROSS)
		SendMouseButton(this, (int)sCursorX, (int)sCursorY, 1, true);
	if (released & PS2_PAD_CROSS)
		SendMouseButton(this, (int)sCursorX, (int)sCursorY, 1, false);
	if (pressed & PS2_PAD_SQUARE)
		SendMouseButton(this, (int)sCursorX, (int)sCursorY, -1, true);
	if (released & PS2_PAD_SQUARE)
		SendMouseButton(this, (int)sCursorX, (int)sCursorY, -1, false);

	SendKeyTransitions(this, pressed, released);

	if (pressed || released || dx != 0.0f || dy != 0.0f)
		mLastUserInputTick = mLastTimerTime;

	return false;
}
