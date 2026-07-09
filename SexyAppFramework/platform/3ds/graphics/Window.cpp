#include <3ds.h>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "widget/WidgetManager.h"

using namespace Sexy;

void SexyAppBase::MakeWindow()
{
	if (mGLInterface == NULL)
	{
		gfxInitDefault();
		C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

		consoleInit(GFX_TOP, NULL);

		mGLInterface = new GLInterface(this);
		InitGLInterface();

		mGLInterface->UpdateViewport();
		mWidgetManager->Resize(mScreenBounds, mGLInterface->mPresentationRect);
	}

	bool isActive = mActive;
	mActive = true;

	mPhysMinimized = false;

	if (isActive != mActive)
		RehupFocus();

	ReInitImages();

	mWidgetManager->mImage = mGLInterface->GetScreenImage();
	mWidgetManager->MarkAllDirty();
}