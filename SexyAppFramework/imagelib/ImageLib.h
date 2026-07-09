#ifndef __IMAGELIB_H__
#define __IMAGELIB_H__

#include <string>

namespace ImageLib
{

class Image
{
public:
	int						mWidth;
	int						mHeight;
	uint32_t*				mBits;
#ifdef PS2_PLATFORM
	uint32_t*				mColorTable;
	unsigned char*			mColorIndices;
	unsigned char*			mRGBBits;
	bool					mHasTrans;
	bool					mHasAlpha;
#endif

public:
	Image();
	virtual ~Image();

	int						GetWidth();
	int						GetHeight();
	uint32_t*				GetBits();
};

bool WriteJPEGImage(const std::string& theFileName, Image* theImage);
bool WritePNGImage(const std::string& theFileName, Image* theImage);
bool WriteTGAImage(const std::string& theFileName, Image* theImage);
bool WriteBMPImage(const std::string& theFileName, Image* theImage);
extern int gAlphaComposeColor;
extern bool gAutoLoadAlpha;
extern bool gIgnoreJPEG2000Alpha;  // I've noticed alpha in jpeg2000's that shouldn't have alpha so this defaults to true


Image* GetImage(const std::string& theFileName, bool lookForAlphaImage = true);

// Reads only the file header to get the decoded dimensions GetImage() would
// produce (including the IMG_DOWNSCALE division and the alpha-only-companion
// fallback), without decoding any pixels. Returns false if no file was found
// or the header could not be parsed.
bool GetImageDims(const std::string& theFileName, int& theWidth, int& theHeight);

//void InitJPEG2000();
//void CloseJPEG2000();
//void SetJ2KCodecKey(const std::string& theKey);

}

#endif //__IMAGELIB_H__
