#define XMD_H

#include "Common.h"
#include "ImageLib.h"
#include "png.h"
#include <math.h>
#include <new>
#include <cstdio>
#include "paklib/PakInterface.h"

#ifdef PS2_PLATFORM
#include <malloc.h>
#include "platform/ps2/Ps2IoLock.h" // printf = fio RPC; serialize with pak IO
#include "SexyAppBase.h"
// Bytes currently handed out by malloc (grows toward the ~32 MB EE ceiling).
static long Ps2UsedHeapBytes()
{
	struct mallinfo mi = mallinfo();
	return (long)mi.uordblks;
}

// Large decode buffers can fail even with free heap left (fragmentation).
// Purge lazily-cached image bits and retry before giving up.
static uint32_t* Ps2AllocImageBits(int theCount)
{
	uint32_t* aBits = new (std::nothrow) uint32_t[theCount];
	if (aBits == NULL && Sexy::gSexyAppBase != NULL)
	{
		printf("[IMG] alloc %d bytes failed (used=%.2fMB), purging bits\n",
			(int)(theCount * sizeof(uint32_t)), (double)Ps2UsedHeapBytes() / (1024.0 * 1024.0));
		Sexy::gSexyAppBase->PurgeLazyImageBits(true);
		aBits = new (std::nothrow) uint32_t[theCount];
		if (aBits == NULL)
		{
			printf("[IMG] alloc still failing, purging textures too\n");
			Sexy::gSexyAppBase->PurgeLazyImages();
			aBits = new (std::nothrow) uint32_t[theCount];
		}
	}
	return aBits;
}

static unsigned char* Ps2AllocImageBytes(int theCount)
{
	unsigned char* aBytes = new (std::nothrow) unsigned char[theCount];
	if (aBytes == NULL && Sexy::gSexyAppBase != NULL)
	{
		printf("[IMG] alloc %d bytes failed (used=%.2fMB), purging bits\n",
			theCount, (double)Ps2UsedHeapBytes() / (1024.0 * 1024.0));
		Sexy::gSexyAppBase->PurgeLazyImageBits(true);
		aBytes = new (std::nothrow) unsigned char[theCount];
		if (aBytes == NULL)
		{
			printf("[IMG] alloc still failing, purging textures too\n");
			Sexy::gSexyAppBase->PurgeLazyImages();
			aBytes = new (std::nothrow) unsigned char[theCount];
		}
	}
	return aBytes;
}
#endif

extern "C"
{
#include "jpeglib.h"
#include "jerror.h"
}

using namespace ImageLib;

Image::Image()
{
	mWidth = 0;
	mHeight = 0;
	mBits = NULL;
#ifdef PS2_PLATFORM
	mColorTable = NULL;
	mColorIndices = NULL;
	mRGBBits = NULL;
	mHasTrans = false;
	mHasAlpha = false;
#endif
}

Image::~Image()
{
	delete[] mBits;
#ifdef PS2_PLATFORM
	delete[] mColorTable;
	delete[] mColorIndices;
	delete[] mRGBBits;
#endif
}

int	Image::GetWidth()
{
	return mWidth;
}

int	Image::GetHeight()
{
	return mHeight;
}

uint32_t* Image::GetBits()
{
#ifdef PS2_PLATFORM
	if (mBits == NULL && ((mColorTable != NULL && mColorIndices != NULL) || mRGBBits != NULL))
	{
		int aSize = mWidth * mHeight;
		mBits = Ps2AllocImageBits(aSize);
		if (mBits == NULL)
			return NULL;

		if (mColorTable != NULL && mColorIndices != NULL)
		{
			for (int i = 0; i < aSize; i++)
				mBits[i] = mColorTable[mColorIndices[i]];
			delete[] mColorTable;
			delete[] mColorIndices;
			mColorTable = NULL;
			mColorIndices = NULL;
		}
		else if (mRGBBits != NULL)
		{
			unsigned char* src = mRGBBits;
			for (int i = 0; i < aSize; i++)
			{
				uint32_t r = *src++;
				uint32_t g = *src++;
				uint32_t b = *src++;
				mBits[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
			}
			delete[] mRGBBits;
			mRGBBits = NULL;
		}
	}
#endif
	return mBits;
}

//////////////////////////////////////////////////////////////////////////
// PNG Pak Support

static void png_pak_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	png_size_t check;

	/* fread() returns 0 on error, so it is OK to store this in a png_size_t
	* instead of an int, which is what fread() actually returns.
	*/
	check = (png_size_t)p_fread(data, (png_size_t)1, length,
		(PFILE*)png_get_io_ptr(png_ptr));

	if (check != length)
	{
		png_error(png_ptr, "Read Error");
	}
}

#ifdef PS2_PLATFORM
// Whole-file PNG source. libpng's streaming callback (png_pak_read_data) issues
// many small p_fread calls per image — a burst of fio SIF RPCs. Loose reanim
// PNGs aren't paked, so each read is a real round-trip; interleaved with the
// mixer thread's continuous audsrv RPC traffic they wedge the shared IOP and
// hard-freeze on real hardware (USB), while emulators/ps2link just run slow.
// Same failure and fix as the JPEG board-load hang (jpeg_whole_buffer_src):
// read the file once, then feed libpng from RAM.
struct Ps2PngMemSource
{
	const unsigned char* mData;
	size_t mSize;
	size_t mPos;
};

static void png_mem_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	Ps2PngMemSource* src = (Ps2PngMemSource*)png_get_io_ptr(png_ptr);
	if (src == NULL || src->mPos + length > src->mSize)
	{
		png_error(png_ptr, "Read Error");
		return;
	}
	memcpy(data, src->mData + src->mPos, length);
	src->mPos += length;
}
#endif

Image* GetPNGImage(const std::string& theFileName)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    PFILE *fp;

    if ((fp = p_fopen(theFileName.c_str(), "rb")) == NULL)
        return NULL;

#ifdef PS2_PLATFORM
    // One fio round-trip for the whole file, then decode from RAM. A local
    // vector frees itself on every return path below (fp is still closed by
    // the existing p_fclose calls). See Ps2PngMemSource above.
    p_fseek(fp, 0, SEEK_END);
    int aPngFileSize = p_ftell(fp);
    p_fseek(fp, 0, SEEK_SET);
    std::vector<unsigned char> aPngFileData;
    Ps2PngMemSource aPngSrc = { NULL, 0, 0 };
    if (aPngFileSize > 0)
    {
        aPngFileData.resize((size_t)aPngFileSize);
        if ((int)p_fread(&aPngFileData[0], 1, aPngFileSize, fp) != aPngFileSize)
        {
            printf("[PNG] whole-file read failed '%s' (%d bytes)\n", theFileName.c_str(), aPngFileSize);
            p_fclose(fp);
            return NULL;
        }
        aPngSrc.mData = &aPngFileData[0];
        aPngSrc.mSize = (size_t)aPngFileSize;
    }
#endif

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL);
#ifdef PS2_PLATFORM
    png_set_read_fn(png_ptr, (png_voidp)&aPngSrc, png_mem_read_data);
#else
    png_set_read_fn(png_ptr, (png_voidp)fp, png_pak_read_data);
#endif

    if (png_ptr == NULL)
    {
        p_fclose(fp);
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        p_fclose(fp);
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        p_fclose(fp);
        return NULL;
    }

    png_read_info(png_ptr, info_ptr);

    int bit_depth, color_type, interlace_type;

    png_get_IHDR(
        png_ptr,
        info_ptr,
        &width,
        &height,
        &bit_depth,
        &color_type,
        &interlace_type,
        NULL,
        NULL);

#ifdef PS2_PLATFORM
    const char* ct = "UNKNOWN";
    switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:       ct = "GRAY"; break;
    case PNG_COLOR_TYPE_GRAY_ALPHA: ct = "GRAY_ALPHA"; break;
    case PNG_COLOR_TYPE_PALETTE:    ct = "PALETTE"; break;
    case PNG_COLOR_TYPE_RGB:        ct = "RGB"; break;
    case PNG_COLOR_TYPE_RGB_ALPHA:  ct = "RGBA"; break;
    }

    Ps2IoLockAcquire();
    printf("[PvZ PS2] PNG '%s' %ux%u type=%s depth=%d\n",
        theFileName.c_str(),
        (unsigned)width,
        (unsigned)height,
        ct,
        bit_depth);
    Ps2IoLockRelease();
#endif

#ifdef PS2_PLATFORM
    bool keepPalette = (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8);
    bool keepRgb = (color_type == PNG_COLOR_TYPE_RGB && bit_depth == 8);
    bool hasTRNS = png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0;
    if (hasTRNS)
    {
        // Palette stays compact: tRNS alpha is baked into the 32-bit color
        // table below. Only the RGB fast path can't carry alpha.
        keepRgb = false;
    }

    if (keepPalette)
    {
        if (bit_depth < 8)
            png_set_packing(png_ptr);
    }
    else if (keepRgb)
    {
        // Keep opaque PNG RGB at 3 bytes/pixel in EE RAM. The PS2 upload path
        // expands only the texture piece being submitted to the GS shim.
    }
    else
#endif
    {
        // Expand indexed/grayscale to RGB
        png_set_expand(png_ptr);
        png_set_palette_to_rgb(png_ptr);
        png_set_gray_to_rgb(png_ptr);

        // Always produce RGBA for compatibility with the current engine.
        png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

        png_set_bgr(png_ptr);
    }

    // Update libpng after the requested conversions.
    png_read_update_info(png_ptr, info_ptr);

    png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

#ifdef PS2_PLATFORM
    Ps2IoLockAcquire();
    printf("[PvZ PS2] Decoded rowbytes=%u channels=%u decode=%.2f MB heapUsed=%.2f MB\n",
        (unsigned)rowbytes,
        (unsigned)(rowbytes / width),
        (double)(rowbytes * height) / (1024.0 * 1024.0),
        (double)Ps2UsedHeapBytes() / (1024.0 * 1024.0));
    Ps2IoLockRelease();
#endif

    png_bytep* row_pointers = new (std::nothrow) png_bytep[height];
    uint8_t* aBits = new (std::nothrow) uint8_t[rowbytes * height];

#ifdef PS2_PLATFORM
    if (aBits == NULL && Sexy::gSexyAppBase != NULL)
    {
        Sexy::gSexyAppBase->PurgeLazyImageBits(true);
        aBits = new (std::nothrow) uint8_t[rowbytes * height];
        if (aBits == NULL)
        {
            Sexy::gSexyAppBase->PurgeLazyImages();
            aBits = new (std::nothrow) uint8_t[rowbytes * height];
        }
    }
    if (row_pointers == NULL && aBits != NULL)
        row_pointers = new (std::nothrow) png_bytep[height];
#endif

    if (row_pointers == NULL || aBits == NULL)
    {
#ifdef PS2_PLATFORM
        printf("[PvZ PS2] GetPNGImage OOM on '%s'\n",
            theFileName.c_str());
#endif
        delete[] row_pointers;
        delete[] aBits;
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        p_fclose(fp);
        return NULL;
    }

    for (uint32_t i = 0; i < height; i++)
        row_pointers[i] = aBits + i * rowbytes;

    png_read_image(png_ptr, row_pointers);

    delete[] row_pointers;

#ifdef PS2_PLATFORM
    uint32_t paletteCopy[256];
    int numPaletteCopy = 0;
    bool aPalHasTrans = false;
    bool aPalHasAlpha = false;
    if (keepPalette)
    {
        for (int i = 0; i < 256; i++)
            paletteCopy[i] = 0xFF000000;
        png_colorp palette = NULL;
        int numPalette = 0;
        png_get_PLTE(png_ptr, info_ptr, &palette, &numPalette);
        png_bytep transAlpha = NULL;
        int numTrans = 0;
        if (hasTRNS)
            png_get_tRNS(png_ptr, info_ptr, &transAlpha, &numTrans, NULL);
        numPaletteCopy = numPalette;
        if (numPaletteCopy > 256)
            numPaletteCopy = 256;
        for (int i = 0; i < numPaletteCopy; i++)
        {
            const png_color& c = palette[i];
            uint32_t anAlpha = (transAlpha != NULL && i < numTrans) ? transAlpha[i] : 0xFF;
            if (anAlpha == 0)
                aPalHasTrans = true;
            else if (anAlpha != 0xFF)
                aPalHasAlpha = true;
            paletteCopy[i] = (anAlpha << 24) |
                ((uint32_t)c.red << 16) |
                ((uint32_t)c.green << 8) |
                (uint32_t)c.blue;
        }
    }
#endif

    png_read_end(png_ptr, info_ptr);

    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

    p_fclose(fp);

    Image* anImage = new Image();
    anImage->mWidth = width;
    anImage->mHeight = height;
#ifdef PS2_PLATFORM
    if (keepPalette)
    {
        anImage->mColorTable = new (std::nothrow) uint32_t[256];
        anImage->mColorIndices = new (std::nothrow) unsigned char[width * height];
        if (anImage->mColorTable != NULL && anImage->mColorIndices != NULL)
        {
            for (int i = 0; i < 256; i++)
                anImage->mColorTable[i] = 0xFF000000;
            memcpy(anImage->mColorTable, paletteCopy, numPaletteCopy * sizeof(uint32_t));
            for (uint32_t y = 0; y < height; y++)
                memcpy(anImage->mColorIndices + y * width, aBits + y * rowbytes, width);
            anImage->mHasTrans = aPalHasTrans;
            anImage->mHasAlpha = aPalHasAlpha;
            delete[] aBits;
        }
        else
        {
            delete[] aBits;
            delete anImage;
            return NULL;
        }
    }
    else if (keepRgb)
    {
        anImage->mRGBBits = aBits;
        anImage->mHasTrans = false;
        anImage->mHasAlpha = false;
    }
    else
#endif
    {
        anImage->mBits = (uint32_t*)aBits;
#ifdef PS2_PLATFORM
        anImage->mHasTrans = false;
        anImage->mHasAlpha = false;
        uint32_t* ptr = anImage->mBits;
        int aSize = anImage->mWidth * anImage->mHeight;
        for (int i = 0; i < aSize; i++)
        {
            unsigned char anAlpha = (unsigned char)(*ptr++ >> 24);
            if (anAlpha == 0)
                anImage->mHasTrans = true;
            else if (anAlpha != 255)
                anImage->mHasAlpha = true;
        }
#endif
    }

    return anImage;
}


Image* GetTGAImage(const std::string& theFileName)
{
	PFILE* aTGAFile = p_fopen(theFileName.c_str(), "rb");
	if (aTGAFile == NULL)
		return NULL;

	BYTE aHeaderIDLen;
	p_fread(&aHeaderIDLen, sizeof(BYTE), 1, aTGAFile);

	BYTE aColorMapType;
	p_fread(&aColorMapType, sizeof(BYTE), 1, aTGAFile);
	
	BYTE anImageType;
	p_fread(&anImageType, sizeof(BYTE), 1, aTGAFile);

	WORD aFirstEntryIdx;
	p_fread(&aFirstEntryIdx, sizeof(WORD), 1, aTGAFile);

	WORD aColorMapLen;
	p_fread(&aColorMapLen, sizeof(WORD), 1, aTGAFile);

	BYTE aColorMapEntrySize;
	p_fread(&aColorMapEntrySize, sizeof(BYTE), 1, aTGAFile);	

	WORD anXOrigin;
	p_fread(&anXOrigin, sizeof(WORD), 1, aTGAFile);

	WORD aYOrigin;
	p_fread(&aYOrigin, sizeof(WORD), 1, aTGAFile);

	WORD anImageWidth;
	p_fread(&anImageWidth, sizeof(WORD), 1, aTGAFile);	

	WORD anImageHeight;
	p_fread(&anImageHeight, sizeof(WORD), 1, aTGAFile);	

	BYTE aBitCount = 32;
	p_fread(&aBitCount, sizeof(BYTE), 1, aTGAFile);	

	BYTE anImageDescriptor = 8 | (1<<5);
	p_fread(&anImageDescriptor, sizeof(BYTE), 1, aTGAFile);

	if ((aBitCount != 32) ||
		(anImageDescriptor != (8 | (1<<5))))
	{
		p_fclose(aTGAFile);
		return NULL;
	}

	Image* anImage = new Image();

	anImage->mWidth = anImageWidth;
	anImage->mHeight = anImageHeight;
#ifdef PS2_PLATFORM
	anImage->mBits = Ps2AllocImageBits(anImageWidth*anImageHeight);
	if (anImage->mBits == NULL)
	{
		delete anImage;
		p_fclose(aTGAFile);
		return NULL;
	}
#else
	anImage->mBits = new uint32_t[anImageWidth*anImageHeight];
#endif

	p_fread(anImage->mBits, 4, anImage->mWidth*anImage->mHeight, aTGAFile);

	p_fclose(aTGAFile);

	return anImage;
}

int ReadBlobBlock(PFILE* fp, char* data)
{
	unsigned char aCount = 0;
	p_fread(&aCount, sizeof(char), 1, fp);
	p_fread(data, sizeof(char), aCount, fp);
	return aCount;
}

Image* GetGIFImage(const std::string& theFileName)
{
	#define BitSet(byte,bit)  (((byte) & (bit)) == (bit))
	#define LSBFirstOrder(x,y)  (((y) << 8) | (x))

	int
		opacity,
		status;

	int i;

	unsigned char *p;

	unsigned char
		background,			// 背景色在全局颜色列表中的索引（背景色：图像中没有被指定颜色的像素会被背景色填充）
		c,
		flag,				// 图像标志的压缩字节
		*global_colormap,	// 全局颜色列表
		header[1664],
		magick[12];

	unsigned int
		delay,
		dispose,
		global_colors,		// 全局颜色列表大小
		image_count,
		iterations;

	/*
	Open image file.
	*/

	PFILE *fp;

	if ((fp = p_fopen(theFileName.c_str(), "rb")) == NULL)
		return NULL;
	/*
	Determine if this is a GIF file.
	*/
	status = p_fread(magick, sizeof(char), 6, fp);  // 读取文件头（包含文件签名与版本号，共 6 字节）
	(void)status; // unused

	// 文件头的 ASCII 值为“GIF87a”或”GIF89a”，其中前三位为 GIF 签名，后三位为不同年份的版本号
	if (((strncmp((char*)magick, "GIF87", 5) != 0) && (strncmp((char*)magick, "GIF89", 5) != 0)))
		return NULL;

	global_colors = 0;
	global_colormap = (unsigned char*)NULL;

	short pw;  // 图像宽度
	short ph;  // 图像高度

	// 读取逻辑屏幕描述符，共 7 字节
	p_fread(&pw, sizeof(short), 1, fp);  // 读取图像渲染区域的宽度
	p_fread(&ph, sizeof(short), 1, fp);  // 读取图像渲染区域的高度
	p_fread(&flag, sizeof(char), 1, fp);  // 读取图像标志
	p_fread(&background, sizeof(char), 1, fp);  // 读取背景色在全局颜色列表中的索引，若无全局颜色列表则此字节无效
	p_fread(&c, sizeof(char), 1, fp);  // 读取像素宽高比

	if (BitSet(flag, 0x80))  // 如果存在全局颜色列表
	{
		/*
		opacity global colormap.
		*/
		global_colors = 1 << ((flag & 0x07) + 1);  // 压缩字节的最低 3 位表示全局颜色列表的大小，设其二进制数值为 N，则列表大小 = 2 ^ (N + 1)
		global_colormap = new unsigned char[3 * global_colors];  // 每个颜色占 3 个字节，按 RGB 排列
		if (global_colormap == (unsigned char*)NULL)
			return NULL;

		p_fread(global_colormap, sizeof(char), 3 * global_colors, fp);  // 读取全局颜色列表
	}

	delay = 0;
	dispose = 0;
	iterations = 1;
	opacity = (-1);
	image_count = 0;

	for (; ; )
	{
		if (p_fread(&c, sizeof(char), 1, fp) == 0)
			break;  // 如果读取错误或读取到文件尾则退出，返回空指针

		if (c == ';')  // 当读取到 gif 结束块标记符（End Of File）
			break;  /* terminator */
		if (c == '!')  // 当读取到 gif 拓展块标记符
		{
			/*
			GIF Extension block.
			*/
			p_fread(&c, sizeof(char), 1, fp);  // 读取拓展块的功能编码号

			switch (c)
			{
			case 0xf9:
			{
				/*
				Read Graphics Control extension.
				*/
				while (ReadBlobBlock(fp, (char*)header) > 0);

				dispose = header[0] >> 2;
				delay = (header[2] << 8) | header[1];
				(void)delay; // Unused
				if ((header[0] & 0x01) == 1)
					opacity = header[3];
				break;
			}
			case 0xfe:
			{
				char* comments;
				int length;

				/*
				Read Comment extension.
				*/
				comments = (char*)NULL;
				for (; ; )
				{
					length = ReadBlobBlock(fp, (char*)header);
					if (length <= 0)
						break;
					if (comments == NULL)
					{
						comments = new char[length + 1];
						if (comments != (char*)NULL)
							*comments = '\0';
					}

					header[length] = '\0';
					strcat(comments, (char*)header);
				}
				if (comments == (char*)NULL)
					break;

				delete comments;
				break;
			}
			case 0xff:
			{
				int
					loop;

				/*
				Read Netscape Loop extension.
				*/
				loop = false;
				if (ReadBlobBlock(fp, (char*)header) > 0)
					loop = !strncmp((char*)header, "NETSCAPE2.0", 11);
				while (ReadBlobBlock(fp, (char*)header) > 0)
					if (loop)
						iterations = (header[2] << 8) | header[1];
				break;
			}
			default:
			{
				while (ReadBlobBlock(fp, (char*)header) > 0);
				break;
			}
			}
		}

		if (c != ',')  // 如果读取的不为图像描述符
			continue;

		if (image_count != 0)
		{
			/*
			Allocate next image structure.
			*/

			/*AllocateNextImage(image_info,image);
			if (image->next == (Image *) NULL)
			{
			DestroyImages(image);
			return((Image *) NULL);
			}
			image=image->next;
			MagickMonitor(LoadImagesText,TellBlob(image),image->filesize);*/
		}
		image_count++;

		short pagex;
		short pagey;
		short width;
		short height;
		int colors;
		bool interlaced;

		p_fread(&pagex, sizeof(short), 1, fp);  // 读取帧的横坐标（Left）
		p_fread(&pagey, sizeof(short), 1, fp);  // 读取帧的纵坐标（Top）
		p_fread(&width, sizeof(short), 1, fp);  // 读取帧的横向宽度（Width）
		p_fread(&height, sizeof(short), 1, fp);  // 取得帧的纵向高度（Height）
		p_fread(&flag, sizeof(char), 1, fp);  // 读取帧标志的压缩字节

		colors = !BitSet(flag, 0x80) ? global_colors : 1 << ((flag & 0x07) + 1);  // 判断使用全局颜色列表或使用局部颜色列表，并取得列表大小
		uint32_t* colortable = new uint32_t[colors];  // 申请颜色列表

		interlaced = BitSet(flag, 0x40);  // 当前帧图像数据存储方式，为 1 表示交织顺序存储，0 表示顺序存储

		delay = 0;
		dispose = 0;
		(void)dispose; // unused
		iterations = 1;
		(void)iterations; //unused
		/*if (image_info->ping)
		{
		f (opacity >= 0)
		/image->matte=true;

		CloseBlob(image);
		return(image);
		}*/
		if ((width == 0) || (height == 0))
			return NULL;
		/*
		Inititialize colormap.
		*/
		/*if (!AllocateImageColormap(image,image->colors))
		ThrowReaderException(ResourceLimitWarning,"Memory allocation failed",
		image);*/
		if (!BitSet(flag, 0x80))  // 如果使用全局颜色列表
		{
			/*
			Use global colormap.
			*/
			p = global_colormap;
			for (i = 0; i < (int)colors; i++)
			{
				int r = *p++;
				int g = *p++;
				int b = *p++;

				colortable[i] = 0xFF000000 | (r << 16) | (g << 8) | (b);
			}

			//image->background_color=
			//image->colormap[Min(background,image->colors-1)];
		}
		else
		{
			unsigned char
				* colormap;

			/*
			Read local colormap.
			*/
			colormap = new unsigned char[3 * colors];

			int pos = p_ftell(fp);
			(void)pos; // unused

			p_fread(colormap, sizeof(char), 3 * colors, fp);

			p = colormap;
			for (i = 0; i < (int)colors; i++)
			{
				int r = *p++;
				int g = *p++;
				int b = *p++;

				colortable[i] = 0xFF000000 | (r << 16) | (g << 8) | (b);
			}
			delete colormap;
		}

		/*if (opacity >= (int) colors)
		{
		for (i=colors; i < (opacity+1); i++)
		{
		image->colormap[i].red=0;
		image->colormap[i].green=0;
		image->colormap[i].blue=0;
		}
		image->colors=opacity+1;
		}*/
		/*
		Decode image.
		*/
		//status=DecodeImage(image,opacity,exception);

		//if (global_colormap != (unsigned char *) NULL)
		// LiberateMemory((void **) &global_colormap);
		if (global_colormap != NULL)
		{
			delete[] global_colormap;
			global_colormap = NULL;
		}

		//while (image->previous != (Image *) NULL)
		//    image=image->previous;

#define MaxStackSize  4096
#define NullCode  (-1)

		int
			available,
			bits,
			code,
			clear,
			code_mask,
			code_size,
			count,
			end_of_information,
			in_code,
			offset,
			old_code,
			pass,
			y;

		int
			x;

		unsigned int
			datum;

		short
			* prefix;

		unsigned char
			data_size,
			first,
			* packet,
			* pixel_stack,
			* suffix,
			* top_stack;

		/*
		Allocate decoder tables.
		*/

		packet = new unsigned char[256];
		prefix = new short[MaxStackSize];
		suffix = new unsigned char[MaxStackSize];
		pixel_stack = new unsigned char[MaxStackSize + 1];

		/*
		Initialize GIF data stream decoder.
		*/
		p_fread(&data_size, sizeof(char), 1, fp);
		clear = 1 << data_size;
		end_of_information = clear + 1;
		available = clear + 2;
		old_code = NullCode;
		code_size = data_size + 1;
		code_mask = (1 << code_size) - 1;
		for (code = 0; code < clear; code++)
		{
			prefix[code] = 0;
			suffix[code] = (unsigned char)code;
		}
		/*
		Decode GIF pixel stream.
		*/
		datum = 0;
		bits = 0;
		c = 0;
		count = 0;
		first = 0;
		offset = 0;
		pass = 0;
		top_stack = pixel_stack;

#ifdef PS2_PLATFORM
		uint32_t* aBits = Ps2AllocImageBits(width * height);
		if (aBits == NULL)
		{
			delete[] packet;
			delete[] prefix;
			delete[] suffix;
			delete[] pixel_stack;
			delete[] colortable;
			p_fclose(fp);
			return NULL;
		}
#else
		uint32_t* aBits = new uint32_t[width * height];
#endif

		unsigned char* c = NULL;

		for (y = 0; y < (int)height; y++)
		{
			//q=SetImagePixels(image,0,offset,width,1);
			//if (q == (PixelPacket *) NULL)
			//break;
			//indexes=GetIndexes(image);

			uint32_t* q = aBits + offset * width;



			for (x = 0; x < (int)width; )
			{
				if (top_stack == pixel_stack)
				{
					if (bits < code_size)
					{
						/*
						Load bytes until there is enough bits for a code.
						*/
						if (count == 0)
						{
							/*
							Read a new data block.
							*/
							int pos = p_ftell(fp);
							(void)pos; // unused

							count = ReadBlobBlock(fp, (char*)packet);
							if (count <= 0)
								break;
							c = packet;
						}
						datum += (*c) << bits;
						bits += 8;
						c++;
						count--;
						continue;
					}
					/*
					Get the next code.
					*/
					code = datum & code_mask;
					datum >>= code_size;
					bits -= code_size;
					/*
					Interpret the code
					*/
					if ((code > available) || (code == end_of_information))
						break;
					if (code == clear)
					{
						/*
						Reset decoder.
						*/
						code_size = data_size + 1;
						code_mask = (1 << code_size) - 1;
						available = clear + 2;
						old_code = NullCode;
						continue;
					}
					if (old_code == NullCode)
					{
						*top_stack++ = suffix[code];
						old_code = code;
						first = (unsigned char)code;
						continue;
					}
					in_code = code;
					if (code >= available)
					{
						*top_stack++ = first;
						code = old_code;
					}
					while (code >= clear)
					{
						*top_stack++ = suffix[code];
						code = prefix[code];
					}
					first = suffix[code];
					/*
					Add a new string to the string table,
					*/
					if (available >= MaxStackSize)
						break;
					*top_stack++ = first;
					prefix[available] = old_code;
					suffix[available] = first;
					available++;
					if (((available & code_mask) == 0) && (available < MaxStackSize))
					{
						code_size++;
						code_mask += available;
					}
					old_code = in_code;
				}
				/*
				Pop a pixel off the pixel stack.
				*/
				top_stack--;

				int index = (*top_stack);

				*q = colortable[index];

				if (index == opacity)
					*q = 0;

				x++;
				q++;
			}

			if (!interlaced)
				offset++;
			else
			{
				switch (pass)
				{
				case 0:
				default:
				{
					offset += 8;
					if (offset >= height)
					{
						pass++;
						offset = 4;
					}
					break;
				}
				case 1:
				{
					offset += 8;
					if (offset >= height)
					{
						pass++;
						offset = 2;
					}
					break;
				}
				case 2:
				{
					offset += 4;
					if (offset >= height)
					{
						pass++;
						offset = 1;
					}
					break;
				}
				case 3:
				{
					offset += 2;
					break;
				}
				}
			}

			if (x < width)
				break;

			/*if (image->previous == (Image *) NULL)
			if (QuantumTick(y,image->rows))
			MagickMonitor(LoadImageText,y,image->rows);*/
		}
		delete pixel_stack;
		delete suffix;
		delete prefix;
		delete packet;

		delete[] colortable;

		//if (y < image->rows)
		//failed = true;

		Image* anImage = new Image();

		anImage->mWidth = width;
		anImage->mHeight = height;
		anImage->mBits = aBits;

		//TODO: Change for animation crap
		p_fclose(fp);
		return anImage;
	}

	p_fclose(fp);

	return NULL;
}

typedef struct my_error_mgr * my_error_ptr;

struct my_error_mgr
{
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);

}

bool ImageLib::WriteJPEGImage(const std::string& theFileName, Image* theImage)
{
	FILE *fp;

	if ((fp = fopen(theFileName.c_str(), "wb")) == NULL)
		return false;

	struct jpeg_compress_struct cinfo;
	struct my_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	if (setjmp(jerr.setjmp_buffer))
	{
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_compress(&cinfo);
		fclose(fp);
		return false;
	}

	jpeg_create_compress(&cinfo);

	cinfo.image_width = theImage->mWidth;
	cinfo.image_height = theImage->mHeight;
	cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    cinfo.optimize_coding = 1;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 80, TRUE);

	jpeg_stdio_dest(&cinfo, fp);

	jpeg_start_compress(&cinfo, true);

	int row_stride = theImage->GetWidth() * 3;

	unsigned char* aTempBuffer = new unsigned char[row_stride];

	uint32_t* aSrcPtr = theImage->mBits;

	for (int aRow = 0; aRow < theImage->mHeight; aRow++)
	{
		unsigned char* aDest = aTempBuffer;

		for (int aCol = 0; aCol < theImage->mWidth; aCol++)
		{
			uint32_t src = *(aSrcPtr++);

			*aDest++ = (src >> 16) & 0xFF;
			*aDest++ = (src >>  8) & 0xFF;
			*aDest++ = (src      ) & 0xFF;
		}

		jpeg_write_scanlines(&cinfo, &aTempBuffer, 1);
	}

	delete [] aTempBuffer;

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	fclose(fp);

	return true;
}

bool ImageLib::WritePNGImage(const std::string& theFileName, Image* theImage)
{
	png_structp png_ptr;
	png_infop info_ptr;

	FILE *fp;

	if ((fp = fopen(theFileName.c_str(), "wb")) == NULL)
		return false;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
	  NULL, NULL, NULL);

	if (png_ptr == NULL)
	{
		fclose(fp);
		return false;
	}

	// Allocate/initialize the memory for image information.  REQUIRED.
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return false;
	}

   // Set error handling if you are using the setjmp/longjmp method (this is
   // the normal method of doing things with libpng).  REQUIRED unless you
   // set up your own error handlers in the png_create_write_struct() earlier.

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		// Free all of the memory associated with the png_ptr and info_ptr
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		// If we get here, we had a problem writeing the file
		return NULL;
	}

	png_init_io(png_ptr, fp);

	png_color_8 sig_bit;
	sig_bit.red = 8;
	sig_bit.green = 8;
	sig_bit.blue = 8;
	/* if the image has an alpha channel then */
	sig_bit.alpha = 8;
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);
	png_set_bgr(png_ptr);

	png_set_IHDR(png_ptr, info_ptr, theImage->mWidth, theImage->mHeight, 8, PNG_COLOR_TYPE_RGB_ALPHA,
       PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	// Add filler (or alpha) byte (before/after each RGB triplet)
	//png_set_expand(png_ptr);
	//png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	//png_set_gray_1_2_4_to_8(png_ptr);
	//png_set_palette_to_rgb(png_ptr);
	//png_set_gray_to_rgb(png_ptr);

	png_write_info(png_ptr, info_ptr);

	for (int i = 0; i < theImage->mHeight; i++)
	{
		png_bytep aRowPtr = (png_bytep) (theImage->mBits + i*theImage->mWidth);
		png_write_rows(png_ptr, &aRowPtr, 1);
	}

	// write rest of file, and get additional chunks in info_ptr - REQUIRED
	png_write_end(png_ptr, info_ptr);

	// clean up after the write, and free any memory allocated - REQUIRED
	png_destroy_write_struct(&png_ptr, &info_ptr);

	// close the file
	fclose(fp);

	return true;
}

bool ImageLib::WriteTGAImage(const std::string& theFileName, Image* theImage)
{
	FILE* aTGAFile = fopen(theFileName.c_str(), "wb");
	if (aTGAFile == NULL)
		return false;

	BYTE aHeaderIDLen = 0;
	fwrite(&aHeaderIDLen, sizeof(BYTE), 1, aTGAFile);

	BYTE aColorMapType = 0;
	fwrite(&aColorMapType, sizeof(BYTE), 1, aTGAFile);
	
	BYTE anImageType = 2;
	fwrite(&anImageType, sizeof(BYTE), 1, aTGAFile);

	WORD aFirstEntryIdx = 0;
	fwrite(&aFirstEntryIdx, sizeof(WORD), 1, aTGAFile);

	WORD aColorMapLen = 0;
	fwrite(&aColorMapLen, sizeof(WORD), 1, aTGAFile);

	BYTE aColorMapEntrySize = 0;
	fwrite(&aColorMapEntrySize, sizeof(BYTE), 1, aTGAFile);	

	WORD anXOrigin = 0;
	fwrite(&anXOrigin, sizeof(WORD), 1, aTGAFile);

	WORD aYOrigin = 0;
	fwrite(&aYOrigin, sizeof(WORD), 1, aTGAFile);

	WORD anImageWidth = theImage->mWidth;
	fwrite(&anImageWidth, sizeof(WORD), 1, aTGAFile);	

	WORD anImageHeight = theImage->mHeight;
	fwrite(&anImageHeight, sizeof(WORD), 1, aTGAFile);	

	BYTE aBitCount = 32;
	fwrite(&aBitCount, sizeof(BYTE), 1, aTGAFile);	

	BYTE anImageDescriptor = 8 | (1<<5);
	fwrite(&anImageDescriptor, sizeof(BYTE), 1, aTGAFile);

	fwrite(theImage->mBits, 4, theImage->mWidth*theImage->mHeight, aTGAFile);

	fclose(aTGAFile);

	return true;
}

#ifndef _WIN32
typedef struct tagBITMAPFILEHEADER {
    uint16_t bfType;
    unsigned int bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    unsigned int bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

using Compression = enum {
    BI_RGB = 0x0000,
    BI_RLE8 = 0x0001,
    BI_RLE4 = 0x0002,
    BI_BITFIELDS = 0x0003,
    BI_JPEG = 0x0004,
    BI_PNG = 0x0005,
    BI_CMYK = 0x000B,
    BI_CMYKRLE8 = 0x000C,
    BI_CMYKRLE4 = 0x000D
};
#endif

bool ImageLib::WriteBMPImage(const std::string& theFileName, Image* theImage)
{
	FILE* aFile = fopen(theFileName.c_str(), "wb");
	if (aFile == NULL)
		return false;

	BITMAPFILEHEADER aFileHeader;
	BITMAPINFOHEADER aHeader;

	memset(&aFileHeader,0,sizeof(aFileHeader));
	memset(&aHeader,0,sizeof(aHeader));

	int aNumBytes = theImage->mWidth*theImage->mHeight*4;

	aFileHeader.bfType = ('M'<<8) | 'B';
	aFileHeader.bfSize = sizeof(aFileHeader) + sizeof(aHeader) + aNumBytes;
	aFileHeader.bfOffBits = sizeof(aHeader); 

	aHeader.biSize = sizeof(aHeader);
	aHeader.biWidth = theImage->mWidth;
	aHeader.biHeight = theImage->mHeight;
	aHeader.biPlanes = 1;
	aHeader.biBitCount = 32;
	aHeader.biCompression = BI_RGB;

	fwrite(&aFileHeader,sizeof(aFileHeader),1,aFile);
	fwrite(&aHeader,sizeof(aHeader),1,aFile);
	uint32_t *aRow = theImage->mBits + (theImage->mHeight-1)*theImage->mWidth;
	int aRowSize = theImage->mWidth*4;
	(void)aRowSize; // Unused
	for (int i=0; i<theImage->mHeight; i++, aRow-=theImage->mWidth)
		fwrite(aRow,4,theImage->mWidth,aFile);

	fclose(aFile);
	return true;
}

////////////////////////////////////////////////////////////////////////// 
// JPEG Pak Reader

typedef struct {
	struct jpeg_source_mgr pub;	/* public fields */

	PFILE * infile;		/* source stream */
	JOCTET * buffer;		/* start of buffer */
	boolean start_of_file;	/* have we gotten any data yet? */
} pak_source_mgr;

typedef pak_source_mgr * pak_src_ptr;

#define INPUT_BUF_SIZE 4096

METHODDEF(void) init_source (j_decompress_ptr cinfo)
{
	pak_src_ptr src = (pak_src_ptr) cinfo->src;
	src->start_of_file = TRUE;
}

METHODDEF(boolean) fill_input_buffer (j_decompress_ptr cinfo)
{
	pak_src_ptr src = (pak_src_ptr) cinfo->src;
	size_t nbytes;

	nbytes = p_fread(src->buffer, 1, INPUT_BUF_SIZE, src->infile);
	//((size_t) fread((void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))

	if (nbytes <= 0) {
		if (src->start_of_file)	/* Treat empty input file as fatal error */
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		WARNMS(cinfo, JWRN_JPEG_EOF);
		/* Insert a fake EOI marker */
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = FALSE;

	return TRUE;
}

METHODDEF(void) skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	pak_src_ptr src = (pak_src_ptr) cinfo->src;

	if (num_bytes > 0) {
		while (num_bytes > (long) src->pub.bytes_in_buffer) {
			num_bytes -= (long) src->pub.bytes_in_buffer;
			(void) fill_input_buffer(cinfo);
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

METHODDEF(void) term_source (j_decompress_ptr /* cinfo */)
{
	/* no work necessary here */
}

#ifdef PS2_PLATFORM
// Whole-buffer source: the file is read with a single p_fread up front.
// Streaming 4KB chunks through the pak reader fires a burst of fio SIF RPCs,
// which collides with the mixer thread's continuous audsrv RPC traffic and
// hard-freezes (see the background*.jpg board-load hang).
METHODDEF(void) mem_init_source (j_decompress_ptr /* cinfo */)
{
}

METHODDEF(boolean) mem_fill_input_buffer (j_decompress_ptr cinfo)
{
	// Only reached past the end of the buffer: fake an EOI so libjpeg
	// finishes instead of reading garbage.
	static JOCTET aFakeEOI[2] = { 0xFF, JPEG_EOI };
	WARNMS(cinfo, JWRN_JPEG_EOF);
	cinfo->src->next_input_byte = aFakeEOI;
	cinfo->src->bytes_in_buffer = 2;
	return TRUE;
}

METHODDEF(void) mem_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	struct jpeg_source_mgr* src = cinfo->src;
	if (num_bytes <= 0)
		return;
	if ((size_t)num_bytes > src->bytes_in_buffer)
		num_bytes = (long)src->bytes_in_buffer;
	src->next_input_byte += (size_t)num_bytes;
	src->bytes_in_buffer -= (size_t)num_bytes;
}

static void jpeg_whole_buffer_src (j_decompress_ptr cinfo, const JOCTET* theBuf, size_t theSize)
{
	if (cinfo->src == NULL)
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			sizeof(struct jpeg_source_mgr));

	struct jpeg_source_mgr* src = cinfo->src;
	src->init_source = mem_init_source;
	src->fill_input_buffer = mem_fill_input_buffer;
	src->skip_input_data = mem_skip_input_data;
	src->resync_to_restart = jpeg_resync_to_restart;
	src->term_source = term_source;
	src->next_input_byte = theBuf;
	src->bytes_in_buffer = theSize;
}
#endif

void jpeg_pak_src (j_decompress_ptr cinfo, PFILE* infile)
{
	pak_src_ptr src;

	/* The source object and input buffer are made permanent so that a series
	* of JPEG images can be read from the same file by calling jpeg_stdio_src
	* only before the first one.  (If we discarded the buffer at the end of
	* one image, we'd likely lose the start of the next one.)
	* This makes it unsafe to use this manager and a different source
	* manager serially with the same JPEG object.  Caveat programmer.
	*/
	if (cinfo->src == NULL) {	/* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			sizeof(pak_source_mgr));
		src = (pak_src_ptr) cinfo->src;
		src->buffer = (JOCTET *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			INPUT_BUF_SIZE * sizeof(JOCTET));
	}

	src = (pak_src_ptr) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->infile = infile;
	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}


Image* GetJPEGImage(const std::string& theFileName)
{
	PFILE *fp;

	if ((fp = p_fopen(theFileName.c_str(), "rb")) == NULL)
		return NULL;

#ifdef PS2_PLATFORM
	// One fio round-trip for the whole file instead of a burst of 4KB reads
	// interleaved with the mixer's audsrv RPCs (see jpeg_whole_buffer_src).
	p_fseek(fp, 0, SEEK_END);
	int aFileSize = p_ftell(fp);
	p_fseek(fp, 0, SEEK_SET);
	unsigned char* aFileData = new (std::nothrow) unsigned char[aFileSize > 0 ? aFileSize : 1];
	if (aFileData == NULL || aFileSize <= 0 ||
		(int)p_fread(aFileData, 1, aFileSize, fp) != aFileSize)
	{
		printf("[JPG] whole-file read failed '%s' (%d bytes)\n", theFileName.c_str(), aFileSize);
		delete [] aFileData;
		p_fclose(fp);
		return NULL;
	}
	p_fclose(fp);
	fp = NULL;
#endif

	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	if (setjmp(jerr.setjmp_buffer))
	{
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
#ifdef PS2_PLATFORM
		printf("[JPG] libjpeg error_exit on '%s'\n", theFileName.c_str());
#endif
		jpeg_destroy_decompress(&cinfo);
#ifdef PS2_PLATFORM
		delete [] aFileData;
#else
		p_fclose(fp);
#endif
		return 0;
	}

	jpeg_create_decompress(&cinfo);
#ifdef PS2_PLATFORM
	jpeg_whole_buffer_src(&cinfo, aFileData, (size_t)aFileSize);
#else
	jpeg_pak_src(&cinfo, fp);
#endif
	jpeg_read_header(&cinfo, TRUE);
#ifdef PS2_PLATFORM
	printf("[JPG] header ok '%s' %ux%u comps=%d\n", theFileName.c_str(),
		cinfo.image_width, cinfo.image_height, cinfo.num_components);
#endif
	jpeg_start_decompress(&cinfo);
#ifdef PS2_PLATFORM
	printf("[JPG] start_decompress ok, allocating %ux%u rgb\n",
		cinfo.output_width, cinfo.output_height);
#endif
	int row_stride = cinfo.output_width * cinfo.output_components;

	unsigned char** buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

#ifdef PS2_PLATFORM
	printf("[JPG] sarray ok buffer=%p row0=%p\n", (void*)buffer, (void*)buffer[0]);
	int aPixelCount = (int)(cinfo.output_width * cinfo.output_height);
	int aRGBBytes = aPixelCount * 3;
	unsigned char* aRGBBits = Ps2AllocImageBytes(aRGBBytes);
	printf("[JPG] rgbbits ok %p (%u bytes)\n", (void*)aRGBBits, (unsigned)aRGBBytes);
	if (aRGBBits == NULL)
	{
		printf("[PS2] GetJPEGImage: OOM on %ux%u rgb\n", cinfo.output_width, cinfo.output_height);
		jpeg_destroy_decompress(&cinfo);
		delete [] aFileData;
		return NULL;
	}
	unsigned char* q = aRGBBits;
#else
	uint32_t* aBits = new uint32_t[cinfo.output_width*cinfo.output_height];
	uint32_t* q = aBits;
#endif

	if (cinfo.output_components==1)
	{
		while (cinfo.output_scanline < cinfo.output_height)
		{
			jpeg_read_scanlines(&cinfo, buffer, 1);

			unsigned char* p = *buffer;
			for (unsigned int i = 0; i < cinfo.output_width; i++)
			{
				int r = *p++;
#ifdef PS2_PLATFORM
				*q++ = (unsigned char)r;
				*q++ = (unsigned char)r;
				*q++ = (unsigned char)r;
#else
				*q++ = 0xFF000000 | (r << 16) | (r << 8) | (r);
#endif
			}
		}
	}
	else
	{
		while (cinfo.output_scanline < cinfo.output_height)
		{
#ifdef PS2_PLATFORM
			if ((cinfo.output_scanline & 127) == 0)
				printf("[JPG] scanline %u/%u q=%p\n", cinfo.output_scanline, cinfo.output_height, (void*)q);
#endif
			jpeg_read_scanlines(&cinfo, buffer, 1);

			unsigned char* p = *buffer;
			for (unsigned int i = 0; i < cinfo.output_width; i++)
			{
				int r = *p++;
				int g = *p++;
				int b = *p++;

#ifdef PS2_PLATFORM
				*q++ = (unsigned char)r;
				*q++ = (unsigned char)g;
				*q++ = (unsigned char)b;
#else
				*q++ = 0xFF000000 | (r << 16) | (g << 8) | (b);
#endif
			}
		}
	}

#ifdef PS2_PLATFORM
	printf("[JPG] scanlines done '%s'\n", theFileName.c_str());
#endif
	Image* anImage = new Image();
	anImage->mWidth = cinfo.output_width;
	anImage->mHeight = cinfo.output_height;
#ifdef PS2_PLATFORM
	anImage->mRGBBits = aRGBBits;
	anImage->mHasTrans = false;
	anImage->mHasAlpha = false;
#else
	anImage->mBits = aBits;
#endif

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

#ifdef PS2_PLATFORM
	delete [] aFileData;
	// Freeze forensics: the real-HW log has ended at "scanlines done" with the
	// USB LED dead. If this line appears, new Image()/libjpeg cleanup/delete[]
	// (heap suspects) are innocent and the wedge is in the NEXT log write.
	printf("[JPG] cleanup ok '%s'\n", theFileName.c_str());
#else
	p_fclose(fp);
#endif

	return anImage;
}

int ImageLib::gAlphaComposeColor = 0xFFFFFF;
// Even on LOW_MEMORY targets (PS2/3DS) the shared main.pak stores separate
// alpha companions ("_Image.png"). Some assets — notably the BrianneTod fonts —
// have ONLY the "_" alpha file and no main image, so disabling this made
// GetImage() return NULL and crashed font loading. The alpha is composited into
// the main image and its copy freed, so the memory cost is transient.
bool ImageLib::gAutoLoadAlpha = true;
bool ImageLib::gIgnoreJPEG2000Alpha = true;

static unsigned char Sample(int w, int h, const unsigned char *pData, int u, int v, int Offset, int ScaleW, int ScaleH, int Bpp)
{
	int Value = 0;
	for(int x = 0; x < ScaleW; x++)
		for(int y = 0; y < ScaleH; y++)
			Value += pData[((v+y)*w+(u+x))*Bpp+Offset];
	return Value/(ScaleW*ScaleH);
}

static unsigned char *Rescale(int Width, int Height, int NewWidth, int NewHeight, const unsigned char *pData)
{
	unsigned char *pTmpData;
	int ScaleW = Width/NewWidth;
	int ScaleH = Height/NewHeight;

	int Bpp = 4;

	pTmpData = new unsigned char[NewWidth*NewHeight*Bpp];

	int c = 0;
	for(int y = 0; y < NewHeight; y++)
		for(int x = 0; x < NewWidth; x++, c++)
		{
			pTmpData[c*Bpp] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 0, ScaleW, ScaleH, Bpp);
			pTmpData[c*Bpp+1] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 1, ScaleW, ScaleH, Bpp);
			pTmpData[c*Bpp+2] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 2, ScaleW, ScaleH, Bpp);
			if(Bpp == 4)
				pTmpData[c*Bpp+3] = Sample(Width, Height, pData, x*ScaleW, y*ScaleH, 3, ScaleW, ScaleH, Bpp);
		}

	return pTmpData;
}

Image* ImageLib::GetImage(const std::string& theFilename, bool lookForAlphaImage)
{
	if (!gAutoLoadAlpha)
		lookForAlphaImage = false;

	int aLastDotPos = theFilename.rfind('.');
	int aLastSlashPos = (int)theFilename.rfind('/');

	std::string anExt;
	std::string aFilename;

	if (aLastDotPos > aLastSlashPos)
	{
		anExt = theFilename.substr(aLastDotPos, theFilename.length() - aLastDotPos);
		aFilename = theFilename.substr(0, aLastDotPos);
	}
	else
		aFilename = theFilename;

	Image* anImage = NULL;

#ifdef PS2_PLATFORM
	// Probe png/jpg first: nearly every asset is one of those, and each miss
	// on an extensionless lookup costs a full (slow) fio open round-trip.
	if ((anImage == NULL) && ((strcasecmp(anExt.c_str(), ".png") == 0) || (anExt.length() == 0)))
		anImage = GetPNGImage(aFilename + ".png");

	if ((anImage == NULL) && ((strcasecmp(anExt.c_str(), ".jpg") == 0) || (anExt.length() == 0)))
		anImage = GetJPEGImage(aFilename + ".jpg");

	if ((anImage == NULL) && ((strcasecmp(anExt.c_str(), ".tga") == 0) || (anExt.length() == 0)))
		anImage = GetTGAImage(aFilename + ".tga");
#else
	if ((anImage == NULL) && ((strcasecmp(anExt.c_str(), ".tga") == 0) || (anExt.length() == 0)))
		anImage = GetTGAImage(aFilename + ".tga");

	if ((anImage == NULL) && ((strcasecmp(anExt.c_str(), ".jpg") == 0) || (anExt.length() == 0)))
		anImage = GetJPEGImage(aFilename + ".jpg");

	if ((anImage == NULL) && ((strcasecmp(anExt.c_str(), ".png") == 0) || (anExt.length() == 0)))
		anImage = GetPNGImage(aFilename + ".png");
#endif

	if ((anImage == NULL) && ((strcasecmp(anExt.c_str(), ".gif") == 0) || (anExt.length() == 0)))
		anImage = GetGIFImage(aFilename + ".gif");

	if ((anImage == NULL) && (strcasecmp(anExt.c_str(), ".j2k") == 0))
		unreachable(); // There are no JPEG2000 files in the project
		//anImage = GetJPEG2000Image(aFilename + ".j2k");
	if ((anImage == NULL) && (strcasecmp(anExt.c_str(), ".jp2") == 0))
		unreachable(); // There are no JPEG2000 files in the project
		//anImage = GetJPEG2000Image(aFilename + ".jp2");


	if (anImage)
	{
		int aNewWidth = anImage->mWidth/IMG_DOWNSCALE;
		int aNewHeight = anImage->mHeight/IMG_DOWNSCALE;
		if (aNewWidth > 0 && aNewHeight > 0 &&
			(aNewWidth != anImage->mWidth || aNewHeight != anImage->mHeight))
		{
			// GetBits() can return NULL on PS2 if the 32-bit expansion OOMs;
			// keep the unscaled image rather than sampling through NULL.
			unsigned char* anOldData = (unsigned char*)anImage->GetBits();
			if (anOldData != NULL)
			{
				unsigned char* aNewData = Rescale(anImage->mWidth, anImage->mHeight, aNewWidth, aNewHeight, anOldData);
				delete[] anImage->mBits;
				anImage->mBits = (uint32_t*)aNewData;
				anImage->mWidth = aNewWidth;
				anImage->mHeight = aNewHeight;
			}
			else
				printf("[IMG] OOM expanding '%s' for downscale; kept full size\n", theFilename.c_str());
		}
	}

	// Check for alpha images
	Image* anAlphaImage = NULL;
	if(lookForAlphaImage)
	{
		// Check _ImageName
		anAlphaImage = GetImage(theFilename.substr(0, aLastSlashPos+1) + "_" +
			theFilename.substr(aLastSlashPos+1, theFilename.length() - aLastSlashPos - 1), false);

		// Check ImageName_
		if(anAlphaImage==NULL)
			anAlphaImage = GetImage(theFilename + "_", false);
	}



	// Compose alpha channel with image. GetBits() can return NULL on PS2 when
	// the compact->32-bit expansion OOMs; composing through it would scribble
	// over low (kernel) memory, which on real hardware is a silent freeze.
	if (anAlphaImage != NULL)
	{
		if (anImage != NULL)
		{
			if ((anImage->mWidth == anAlphaImage->mWidth) &&
				(anImage->mHeight == anAlphaImage->mHeight))
			{
				uint32_t* aBits1 = anImage->GetBits();
				uint32_t* aBits2 = anAlphaImage->GetBits();
				if (aBits1 != NULL && aBits2 != NULL)
				{
					int aSize = anImage->mWidth*anImage->mHeight;

					for (int i = 0; i < aSize; i++)
					{
						*aBits1 = (*aBits1 & 0x00FFFFFF) | ((*aBits2 & 0xFF) << 24);
						++aBits1;
						++aBits2;
					}
				}
				else
					printf("[IMG] OOM expanding '%s' for alpha compose; alpha dropped\n", theFilename.c_str());
			}

			delete anAlphaImage;
		}
		else
		{
			anImage = anAlphaImage;

			// Freeze forensics (real HW): the log has ended right after this
			// companion's decode. These prints bracket the two remaining ops
			// (GetBits expansion alloc + compose loop) so the log names the
			// exact one that dies.
			printf("[IMG] alpha-only '%s': expanding %dx%d\n",
				theFilename.c_str(), anImage->mWidth, anImage->mHeight);
			uint32_t* aBits1 = anImage->GetBits();
			printf("[IMG] alpha-only '%s': expand %s\n",
				theFilename.c_str(), (aBits1 != NULL) ? "ok, composing" : "FAILED (OOM)");
			if (aBits1 == NULL)
			{
				delete anImage;
				return NULL;
			}

			int aSize = anImage->mWidth*anImage->mHeight;
			if (gAlphaComposeColor==0xFFFFFF)
			{
				for (int i = 0; i < aSize; i++)
				{
					*aBits1 = (0x00FFFFFF) | ((*aBits1 & 0xFF) << 24);
					++aBits1;
				}
			}
			else
			{
				const int aColor = gAlphaComposeColor;
				for (int i = 0; i < aSize; i++)
				{
					*aBits1 = aColor | ((*aBits1 & 0xFF) << 24);
					++aBits1;
				}
			}
			printf("[IMG] alpha-only '%s': compose done\n", theFilename.c_str());
		}
	}

	return anImage;
}

///////////////////////////////////////////////////////////////////////////////
// Header-only dimension readers (see GetImageDims in ImageLib.h)
///////////////////////////////////////////////////////////////////////////////

static bool GetPNGDims(const std::string& theFileName, int& theWidth, int& theHeight)
{
	PFILE* fp = p_fopen(theFileName.c_str(), "rb");
	if (fp == NULL)
		return false;

	// 8-byte signature, IHDR length+type (8), then width/height big-endian.
	unsigned char aHeader[24];
	bool aOk = p_fread(aHeader, 1, 24, fp) == 24 &&
		aHeader[0] == 0x89 && aHeader[1] == 'P' && aHeader[2] == 'N' && aHeader[3] == 'G';
	p_fclose(fp);
	if (!aOk)
		return false;

	theWidth = (aHeader[16] << 24) | (aHeader[17] << 16) | (aHeader[18] << 8) | aHeader[19];
	theHeight = (aHeader[20] << 24) | (aHeader[21] << 16) | (aHeader[22] << 8) | aHeader[23];
	return theWidth > 0 && theHeight > 0;
}

static bool GetJPEGDims(const std::string& theFileName, int& theWidth, int& theHeight)
{
	PFILE* fp = p_fopen(theFileName.c_str(), "rb");
	if (fp == NULL)
		return false;

	unsigned char aBuf[4];
	if (p_fread(aBuf, 1, 2, fp) != 2 || aBuf[0] != 0xFF || aBuf[1] != 0xD8)
	{
		p_fclose(fp);
		return false;
	}

	// Walk the marker segments until a start-of-frame (SOF0..SOF15, except
	// DHT/DAC/RSTn) which carries precision(1), height(2), width(2).
	bool aFound = false;
	while (p_fread(aBuf, 1, 2, fp) == 2)
	{
		if (aBuf[0] != 0xFF)
			break;
		unsigned char aMarker = aBuf[1];
		if (aMarker == 0xFF)  // fill byte
		{
			p_fseek(fp, -1, SEEK_CUR);
			continue;
		}
		if (aMarker == 0xD8 || (aMarker >= 0xD0 && aMarker <= 0xD7) || aMarker == 0x01)
			continue;  // standalone markers, no length
		if (p_fread(aBuf, 1, 2, fp) != 2)
			break;
		int aLen = (aBuf[0] << 8) | aBuf[1];
		if (aMarker >= 0xC0 && aMarker <= 0xCF && aMarker != 0xC4 && aMarker != 0xC8 && aMarker != 0xCC)
		{
			unsigned char aSof[5];
			if (p_fread(aSof, 1, 5, fp) == 5)
			{
				theHeight = (aSof[1] << 8) | aSof[2];
				theWidth = (aSof[3] << 8) | aSof[4];
				aFound = theWidth > 0 && theHeight > 0;
			}
			break;
		}
		if (aLen < 2)
			break;
		p_fseek(fp, aLen - 2, SEEK_CUR);
	}

	p_fclose(fp);
	return aFound;
}

static bool GetTGADims(const std::string& theFileName, int& theWidth, int& theHeight)
{
	PFILE* fp = p_fopen(theFileName.c_str(), "rb");
	if (fp == NULL)
		return false;

	unsigned char aHeader[18];
	bool aOk = p_fread(aHeader, 1, 18, fp) == 18;
	p_fclose(fp);
	if (!aOk)
		return false;

	theWidth = aHeader[12] | (aHeader[13] << 8);
	theHeight = aHeader[14] | (aHeader[15] << 8);
	return theWidth > 0 && theHeight > 0;
}

static bool GetGIFDims(const std::string& theFileName, int& theWidth, int& theHeight)
{
	PFILE* fp = p_fopen(theFileName.c_str(), "rb");
	if (fp == NULL)
		return false;

	unsigned char aHeader[10];
	bool aOk = p_fread(aHeader, 1, 10, fp) == 10 &&
		aHeader[0] == 'G' && aHeader[1] == 'I' && aHeader[2] == 'F';
	p_fclose(fp);
	if (!aOk)
		return false;

	theWidth = aHeader[6] | (aHeader[7] << 8);
	theHeight = aHeader[8] | (aHeader[9] << 8);
	return theWidth > 0 && theHeight > 0;
}

// Mirrors GetImage()'s extension probing so the stub dimensions always match
// what a later decode produces.
static bool GetAnyImageDims(const std::string& theFilename, int& theWidth, int& theHeight)
{
	int aLastDotPos = theFilename.rfind('.');
	int aLastSlashPos = (int)theFilename.rfind('/');

	std::string anExt;
	std::string aFilename;

	if (aLastDotPos > aLastSlashPos)
	{
		anExt = theFilename.substr(aLastDotPos, theFilename.length() - aLastDotPos);
		aFilename = theFilename.substr(0, aLastDotPos);
	}
	else
		aFilename = theFilename;

#ifdef PS2_PLATFORM
	// Same probe order as GetImage: png/jpg first to cut fio misses.
	if (((strcasecmp(anExt.c_str(), ".png") == 0) || (anExt.length() == 0)) && GetPNGDims(aFilename + ".png", theWidth, theHeight))
		return true;
	if (((strcasecmp(anExt.c_str(), ".jpg") == 0) || (anExt.length() == 0)) && GetJPEGDims(aFilename + ".jpg", theWidth, theHeight))
		return true;
	if (((strcasecmp(anExt.c_str(), ".tga") == 0) || (anExt.length() == 0)) && GetTGADims(aFilename + ".tga", theWidth, theHeight))
		return true;
#else
	if (((strcasecmp(anExt.c_str(), ".tga") == 0) || (anExt.length() == 0)) && GetTGADims(aFilename + ".tga", theWidth, theHeight))
		return true;
	if (((strcasecmp(anExt.c_str(), ".jpg") == 0) || (anExt.length() == 0)) && GetJPEGDims(aFilename + ".jpg", theWidth, theHeight))
		return true;
	if (((strcasecmp(anExt.c_str(), ".png") == 0) || (anExt.length() == 0)) && GetPNGDims(aFilename + ".png", theWidth, theHeight))
		return true;
#endif
	if (((strcasecmp(anExt.c_str(), ".gif") == 0) || (anExt.length() == 0)) && GetGIFDims(aFilename + ".gif", theWidth, theHeight))
		return true;
	return false;
}

bool ImageLib::GetImageDims(const std::string& theFileName, int& theWidth, int& theHeight)
{
	bool aFound = GetAnyImageDims(theFileName, theWidth, theHeight);

	// Alpha-only assets (only "_Name" / "Name_" exists): GetImage() builds the
	// image from the alpha companion, so take the dimensions from it.
	if (!aFound && gAutoLoadAlpha)
	{
		int aLastSlashPos = (int)theFileName.rfind('/');
		aFound = GetAnyImageDims(theFileName.substr(0, aLastSlashPos + 1) + "_" +
			theFileName.substr(aLastSlashPos + 1, theFileName.length() - aLastSlashPos - 1), theWidth, theHeight);
		if (!aFound)
			aFound = GetAnyImageDims(theFileName + "_", theWidth, theHeight);
	}

	if (!aFound)
		return false;

	// Match GetImage()'s downscale exactly.
	int aNewWidth = theWidth / IMG_DOWNSCALE;
	int aNewHeight = theHeight / IMG_DOWNSCALE;
	if (aNewWidth > 0 && aNewHeight > 0)
	{
		theWidth = aNewWidth;
		theHeight = aNewHeight;
	}
	return true;
}
