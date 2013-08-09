/*!
 * \file test_gif.c
 * \brief data pool test
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The external gif test
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */
#include "../hd_test_config.h"
#if TEST_GIF

/*****************************************************************************
 *   "Gif-Lib" - Yet another gif library.				     *
 *									     *
 * Written by:  Gershon Elber				Ver 0.1, Jun. 1989   *
 ******************************************************************************
 * Module to conver raw image into a GIF file.				     *
 * Options:                                                                   *
 * -q : quiet printing mode.						     *
 * -s Width Height : specifies size of raw image.                             *
 * -p ColorMapFile : specifies color map for ray image (see gifclrmp).        *
 * -h : on-line help.                                                         *
 ******************************************************************************
 * History:								     *
 * 15 Oct 89 - Version 1.0 by Gershon Elber.				     *
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __MSDOS__
#include <dos.h>
#include <alloc.h>
#include <stdlib.h>
#include <graphics.h>
#include <io.h>
#else
#include <stdlib.h>
#endif

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <gif/gif_lib.h>

#define PROGRAM_NAME	"Raw2Gif"
#define GIF_MESSAGE(Msg) fprintf(stderr, "\n%s: %s\n", PROGRAM_NAME, Msg)
#define GIF_EXIT(Msg)    { GIF_MESSAGE(Msg); exit(-3); }

extern void GifQprintf(char *Format, ...);

#define FALSE 0

#ifdef __MSDOS__
extern unsigned int
_stklen = 16384; /* Increase default stack size. */
#endif /* __MSDOS__ */

static GifColorType EGAPalette[] = /* Default color map is EGA palette. */
{
		{ 0, 0, 0 }, /* 0. Black */
		{ 0, 0, 170 }, /* 1. Blue */
		{ 0, 170, 0 }, /* 2. Green */
		{ 0, 170, 170 }, /* 3. Cyan */
		{ 170, 0, 0 }, /* 4. Red */
		{ 170, 0, 170 }, /* 5. Magenta */
		{ 170, 170, 0 }, /* 6. Brown */
		{ 170, 170, 170 }, /* 7. LightGray */
		{ 85, 85, 85 }, /* 8. DarkGray */
		{ 85, 85, 255 }, /* 9. LightBlue */
		{ 85, 255, 85 }, /* 10. LightGreen */
		{ 85, 255, 255 }, /* 11. LightCyan */
		{ 255, 85, 85 }, /* 12. LightRed */
		{ 255, 85, 255 }, /* 13. LightMagenta */
		{ 255, 255, 85 }, /* 14. Yellow */
		{ 255, 255, 255 }, /* 15. White */
};
#define EGA_PALETTE_SIZE (sizeof(EGAPalette) / sizeof(GifColorType))

int Raw2Gif(int ImagwWidth, int ImagwHeight, ColorMapObject *ColorMap);
int CreateGif(int ImageWidth, int ImageHeight, int frame_num, ColorMapObject *ColorMap);
static int HandleGifError(GifFileType *GifFile);

/******************************************************************************
 * Interpret the command line, prepar global data and call the Gif routines.   *
 ******************************************************************************/
int main(int argc, char **argv)
{
	int ImageWidth = 10, ImageHeight = 10;
	ColorMapObject *ColorMap;

	ColorMap = GifMakeMapObject(16, EGAPalette);
	/* Conver Raw image from stdin to Gif file in stdout: */
	CreateGif(ImageWidth, ImageHeight, 15, ColorMap);

	GifFreeMapObject(ColorMap);
	return 0;
}

static int
EGifPutExtensions(GifFileType *GifFileOut,
		ExtensionBlock *ExtensionBlocks,
		int ExtensionBlockCount)
{
	if (ExtensionBlocks) {
		ExtensionBlock *ep;
		int j;
		for (j = 0; j < ExtensionBlockCount; j++) {
			ep = &ExtensionBlocks[j];
			if (ep->Function != CONTINUE_EXT_FUNC_CODE)
				if (EGifPutExtensionLeader(GifFileOut, ep->Function) == GIF_ERROR)
					return (GIF_ERROR);
			if (EGifPutExtensionBlock(GifFileOut, ep->ByteCount, ep->Bytes) == GIF_ERROR)
				return (GIF_ERROR);
			if (j == ExtensionBlockCount - 1|| (ep+1)->Function != CONTINUE_EXT_FUNC_CODE)
					if (EGifPutExtensionTrailer(GifFileOut) == GIF_ERROR)
					return (GIF_ERROR);
		}
	}
	return (GIF_OK);
}

int CreateGif(int ImageWidth, int ImageHeight, int frame_num, ColorMapObject *ColorMap)
{
	int i, j, frame;
	static GifPixelType *ScanLine;
	GifPixelType color, dcolor;
	GifFileType *GifFile;
	GraphicsControlBlock gcb;
	int LeadingExtensionBlockCount = 0;
	ExtensionBlock *LeadingExtensionBlocks = NULL;
	int ErrorCode;
	size_t Len;
	char buf[BUFSIZ * 2];
	int intval = 0;

	if ((ScanLine = (GifPixelType *) malloc(sizeof(GifPixelType) * ImageWidth))
			== NULL) {
		GIF_MESSAGE("Failed to allocate scan line, aborted.");
		exit(EXIT_FAILURE);
	}

	if ((GifFile = EGifOpenFileHandle(1, &ErrorCode)) == NULL) { /* Gif to stdout. */
		free((char *) ScanLine);
		return HandleGifError(GifFile);
	}

	if (EGifPutScreenDesc(GifFile, ImageWidth, ImageHeight, ColorMap->BitsPerPixel,
							0, ColorMap) == GIF_ERROR) {
		free((char *) ScanLine);
		return HandleGifError(GifFile);
	}

	unsigned char params[3] = { 1, 0, 0 };
	/* Create a Netscape 2.0 loop block */
	if (GifAddExtensionBlock(&LeadingExtensionBlockCount,
								&LeadingExtensionBlocks,
								APPLICATION_EXT_FUNC_CODE,
								11,
								(unsigned char *) "NETSCAPE2.0") == GIF_ERROR) {
		GIF_MESSAGE("out of memory while adding loop block.");
		exit(EXIT_FAILURE);
	}
	params[1] = (intval & 0xff);
	params[2] = (intval >> 8) & 0xff;
	if (GifAddExtensionBlock(&LeadingExtensionBlockCount,
								&LeadingExtensionBlocks,
								0, sizeof(params), params) == GIF_ERROR) {
		GIF_MESSAGE("out of memory while adding loop continuation.");
		exit(EXIT_FAILURE);
	}

	//ADD CONTROL BLOCK
	gcb.TransparentColor = NO_TRANSPARENT_COLOR;
	gcb.DelayTime = 5;
	gcb.DisposalMode = DISPOSE_DO_NOT;
	gcb.UserInputFlag = 0;
	Len = EGifGCBToExtension(&gcb, (GifByteType *) buf);
	if (GifAddExtensionBlock(&LeadingExtensionBlockCount,
								&LeadingExtensionBlocks,
								GRAPHICS_EXT_FUNC_CODE,
								Len,
								(unsigned char *) buf) == GIF_ERROR) {
		GIF_MESSAGE("out of memory while adding GCB.");
		exit(EXIT_FAILURE);
	}

	EGifPutExtensions(GifFile, LeadingExtensionBlocks, LeadingExtensionBlockCount);

	/* Here it is - create one frame and add to gif. */
	dcolor = ColorMap->ColorCount / frame_num;
	color = 0;
	for (frame = 0; frame < frame_num; frame++) {
		GifQprintf("\n%s: frame %d", PROGRAM_NAME, frame);

		//ADD NEW IMAGE
		if (EGifPutImageDesc(GifFile, 0, 0, ImageWidth, ImageHeight, FALSE,
								NULL) == GIF_ERROR) {
			free((char *) ScanLine);
			return HandleGifError(GifFile);
		}

		/* set pixel   */
		color += dcolor;
		memset(ScanLine, color, ImageWidth);
		for (i = 0; i < ImageHeight; i++) {
			for (j = 0; j < ImageWidth; j++)
				if (ScanLine[j] >= ColorMap->ColorCount)
					GIF_MESSAGE("Warning: RAW data color > maximum color map entry.");

			if (EGifPutLine(GifFile, ScanLine, ImageWidth) == GIF_ERROR) {
				free((char *) ScanLine);
				return HandleGifError(GifFile);
			}
			GifQprintf("\b\b\b\b%-4d", i);
		}
	}

	if (EGifCloseFile(GifFile) == GIF_ERROR) {
		free((char *) ScanLine);
		return HandleGifError(GifFile);
	}

	free((char *) ScanLine);
	return 0;
}

/******************************************************************************
 * Convert Raw image (One byte per pixel) into Gif file. Raw data is read from *
 * stdin, and Gif is dumped to stdout. ImagwWidth times ImageHeight bytes are  *
 * read. Color map is dumped from ColorMap.				      *
 ******************************************************************************/
int Raw2Gif(int ImageWidth, int ImageHeight, ColorMapObject *ColorMap)
{
	int i, j;
	static GifPixelType *ScanLine;
	GifFileType *GifFile;
	int ErrorCode;

	if ((ScanLine = (GifPixelType *) malloc(sizeof(GifPixelType) * ImageWidth))
			== NULL) {
		GIF_MESSAGE("Failed to allocate scan line, aborted.");
		exit(EXIT_FAILURE);
	}

	if ((GifFile = EGifOpenFileHandle(1, &ErrorCode)) == NULL) { /* Gif to stdout. */
		free((char *) ScanLine);
		return HandleGifError(GifFile);
	}

	if (EGifPutScreenDesc(GifFile, ImageWidth, ImageHeight, ColorMap->BitsPerPixel,
							0, ColorMap) == GIF_ERROR) {
		free((char *) ScanLine);
		return HandleGifError(GifFile);
	}

	if (EGifPutImageDesc(GifFile, 0, 0, ImageWidth, ImageHeight, FALSE,
							NULL) == GIF_ERROR) {
		free((char *) ScanLine);
		return HandleGifError(GifFile);
	}

	/* Here it is - get one raw line from stdin, and dump to stdout Gif: */
	GifQprintf("\n%s: Image 1 at (0, 0) [%dx%d]:     ",
				PROGRAM_NAME, ImageWidth, ImageHeight);
	for (i = 0; i < ImageHeight; i++) {
		/* Note we assume here PixelSize == Byte, which is not necessarily   */
		/* so. If not - must read one byte at a time, and coerce to pixel.   */
		if (fread(ScanLine, 1, ImageWidth, stdin) != (unsigned) ImageWidth) {
			GIF_MESSAGE("RAW input file ended prematurely.");
			exit(EXIT_FAILURE);
		}

		for (j = 0; j < ImageWidth; j++)
			if (ScanLine[j] >= ColorMap->ColorCount)
				GIF_MESSAGE("Warning: RAW data color > maximum color map entry.");

		if (EGifPutLine(GifFile, ScanLine, ImageWidth) == GIF_ERROR) {
			free((char *) ScanLine);
			return HandleGifError(GifFile);
		}
		GifQprintf("\b\b\b\b%-4d", i);
	}

	if (EGifCloseFile(GifFile) == GIF_ERROR) {
		free((char *) ScanLine);
		return HandleGifError(GifFile);
	}

	free((char *) ScanLine);
	return 0;
}

/******************************************************************************
 * Handle last GIF error. Try to close the file and free all allocated memory. *
 ******************************************************************************/
static int HandleGifError(GifFileType *GifFile)
{
	int i = GifFile->Error;

	if (EGifCloseFile(GifFile) == GIF_ERROR) {
		i = GifFile->Error;
	}
	return i;
}

bool GifNoisyPrint = false;

/*****************************************************************************
 Same as fprintf to stderr but with optional print.
 ******************************************************************************/
void
GifQprintf(char *Format, ...) {
	char Line[128];
	va_list ArgPtr;

	va_start(ArgPtr, Format);

	if (GifNoisyPrint) {
		(void) vsnprintf(Line, sizeof(Line), Format, ArgPtr);
		(void) fputs(Line, stderr);
	}

	va_end(ArgPtr);
}

void
PrintGifError(int ErrorCode) {
	char *Err = GifErrorString(ErrorCode);

	if (Err != NULL)
		fprintf(stderr, "GIF-LIB error: %s.\n", Err);
	else
		fprintf(stderr, "GIF-LIB undefined error %d.\n", ErrorCode);
}

#endif
