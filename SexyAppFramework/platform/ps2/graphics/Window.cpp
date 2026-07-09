#include <dmaKit.h>
#include <graph.h>
#include <gsKit.h>
#include <gsMisc.h>
#include <sifrpc.h>
#include <stdio.h>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "widget/WidgetManager.h"

using namespace Sexy;

extern GSGLOBAL* gsGlobal;

static void InitPs2GsKit()
{
	static bool sInited = false;
	if (sInited)
		return;

	SifInitRpc(0);
	dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
		D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
	dmaKit_chan_init(DMA_CHANNEL_GIF);

	gsGlobal = gsKit_init_global();
	bool isPal = graph_get_region() == GRAPH_MODE_PAL;
	gsGlobal->Mode = isPal ? GS_MODE_PAL : GS_MODE_NTSC;
	// FRAME (not FIELD): with FIELD rendering, anything that moves shears into
	// horizontal comb strips (visible on the drifting menu clouds). FRAME mode
	// renders/display the full progressive buffer per vsync.
	gsGlobal->Interlace = GS_INTERLACED;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 640;
	gsGlobal->Height = isPal ? 256 : 224;
	// CT32 framebuffer: a CT16 target quantizes the final image to 5 bits per
	// channel, which banded the sky gradients no matter the texture quality.
	gsGlobal->PSM = GS_PSM_CT32;
	gsGlobal->PSMZ = GS_PSMZ_16S;
	gsGlobal->DoubleBuffering = GS_SETTING_ON;
	gsGlobal->ZBuffering = GS_SETTING_OFF;

	gsKit_init_screen(gsGlobal);
	gsKit_mode_switch(gsGlobal, GS_ONESHOT);
	gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0, 0, 0, 0x80, 0));
	gsKit_queue_exec(gsGlobal);
	gsKit_sync_flip(gsGlobal);
	if (gsGlobal->Os_Queue)
		gsKit_queue_reset(gsGlobal->Os_Queue);

	printf("[PS2] gsKit initialized: %dx%d %s\n",
		(int)gsGlobal->Width, (int)gsGlobal->Height, isPal ? "PAL" : "NTSC");
	sInited = true;
}

void SexyAppBase::MakeWindow()
{
	if (!mWindow)
	{
		InitPs2GsKit();
		mWindow = (void*)gsGlobal;
		mContext = (void*)gsGlobal;
	}

	if (mGLInterface == NULL)
	{
		printf("[PS2] creating GLInterface\n");
		mGLInterface = new GLInterface(this);
		printf("[PS2] InitGLInterface begin\n");
		InitGLInterface();
		printf("[PS2] InitGLInterface done\n");
	}

	bool isActive = mActive;
	mActive = true;

	mPhysMinimized = false;
	if (mMinimized)
	{
		if (mMuteOnLostFocus)
			Unmute(true);

		mMinimized = false;
		isActive = mActive; // set this here so we don't call RehupFocus again.
		RehupFocus();
	}
	
	if (isActive != mActive)
		RehupFocus();

	ReInitImages();

	mWidgetManager->mImage = mGLInterface->GetScreenImage();
	mWidgetManager->MarkAllDirty();

	mGLInterface->UpdateViewport();
	printf("[PS2] viewport updated\n");
	mWidgetManager->Resize(mScreenBounds, mGLInterface->mPresentationRect);
}
