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
#include <gif/gif_api.h>

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

e_int32 gif_open(gif_t * gif, char *file_name, e_uint32 width, e_uint32 height,
		e_int32 mode) {
	GifFileType *GifFile;
	int ErrorCode;
	ColorMapObject *ColorMap;

	e_assert(gif, E_ERROR_INVALID_HANDLER);
	memset(gif, 0, sizeof(gif_t));

	if (mode == E_WRITE) {
		GifFile = EGifOpenFileName(file_name, FALSE, &ErrorCode);
		EGifSetGifVersion(GifFile,1);
		e_assert(GifFile, E_ERROR_INVALID_CALL);
		ColorMap = GifMakeMapObject(16, EGAPalette);
		if (EGifPutScreenDesc(GifFile, width, height, ColorMap->BitsPerPixel,
								0, ColorMap) == GIF_ERROR)
		{
			GifFreeMapObject(ColorMap);
			EGifCloseFile(GifFile);
			return E_ERROR_INVALID_CALL;
		}
		GifFreeMapObject(ColorMap);
		gif->width = width;
		gif->height = height;
	} else {
		GifFile = DGifOpenFileName(file_name, &ErrorCode);
		e_assert(GifFile, E_ERROR_INVALID_CALL);
	}

	strcpy(gif->file_name, file_name);
	gif->handle = (size_t) GifFile;
	gif->mode = mode;
	gif->state = 1;

	return E_OK;
}
e_int32 gif_close(gif_t * gif) {
	GifFileType *GifFile;
	e_int32 ret;
	e_assert(gif&&gif->state, E_ERROR_INVALID_HANDLER);
	GifFile = (GifFileType *) gif->handle;
	ret = EGifCloseFile(GifFile);
	if (GIF_ERROR == ret)
		ret = DGifCloseFile(GifFile);
	e_assert(GIF_ERROR != ret, E_ERROR);
	return E_OK;
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

e_int32 gif_put_header(gif_t * gif) {
	GifFileType *GifFile;
	e_int32 ret;
	GraphicsControlBlock gcb;
	int LeadingExtensionBlockCount = 0;
	ExtensionBlock *LeadingExtensionBlocks = NULL;
	size_t Len;
	char buf[BUFSIZ * 2];
	int intval = 0;
	unsigned char params[3] = { 1, 0, 0 };

	e_assert(gif&&gif->state, E_ERROR_INVALID_HANDLER);
	GifFile = (GifFileType *) gif->handle;

	/* Create a Netscape 2.0 loop block */
	ret = GifAddExtensionBlock(&LeadingExtensionBlockCount,
								&LeadingExtensionBlocks,
								APPLICATION_EXT_FUNC_CODE,
								11,
								(unsigned char *) "NETSCAPE2.0");
	e_assert(ret != GIF_ERROR, E_ERROR_INVALID_CALL);

	params[1] = (intval & 0xff);
	params[2] = (intval >> 8) & 0xff;

	ret = GifAddExtensionBlock(&LeadingExtensionBlockCount,
								&LeadingExtensionBlocks,
								0, sizeof(params), params);
	e_assert(ret != GIF_ERROR, E_ERROR_INVALID_CALL);

	//ADD CONTROL BLOCK
	gcb.TransparentColor = NO_TRANSPARENT_COLOR;
	gcb.DelayTime = 5;
	gcb.DisposalMode = DISPOSE_DO_NOT;
	gcb.UserInputFlag = 0;
	Len = EGifGCBToExtension(&gcb, (GifByteType *) buf);

	ret = GifAddExtensionBlock(&LeadingExtensionBlockCount,
								&LeadingExtensionBlocks,
								GRAPHICS_EXT_FUNC_CODE,
								Len,
								(unsigned char *) buf);
	e_assert(ret != GIF_ERROR, E_ERROR_INVALID_CALL);

	ret = EGifPutExtensions(GifFile, LeadingExtensionBlocks, LeadingExtensionBlockCount);
	e_assert(ret != GIF_ERROR, E_ERROR_INVALID_CALL);
	GifFreeExtensions(&LeadingExtensionBlockCount,&LeadingExtensionBlocks);
	return E_OK;
}
e_int32 gif_put_image(gif_t * gif) {
	GifFileType *GifFile;
	e_int32 ret;

	e_assert(gif&&gif->state, E_ERROR_INVALID_HANDLER);
	GifFile = (GifFileType *) gif->handle;

	//ADD NEW IMAGE
	ret = EGifPutImageDesc(GifFile, 0, 0, gif->width, gif->height, FALSE,
							NULL);
	e_assert(ret != GIF_ERROR, E_ERROR_INVALID_CALL);
	return E_OK;
}
e_int32 gif_put_scan_line(gif_t * gif, e_uint8* line) {
	GifFileType *GifFile;
	e_int32 ret;

	e_assert(gif&&gif->state, E_ERROR_INVALID_HANDLER);
	GifFile = (GifFileType *) gif->handle;

	ret = EGifPutLine(GifFile, (GifPixelType*) line, gif->width);
	e_assert(ret != GIF_ERROR, E_ERROR_INVALID_CALL);
	return E_OK;
}

e_int32
gif_union(gif_t *gif, gif_t * gif1, gif_t * gif2)
{
	GifFileType *GifFile1, *GifFile2, *GifFile;
	int error_code, ret, i, j, count;
	char file_name[MAX_PATH_LEN];
	int width, height;
	e_uint8* line=NULL, *ptr1, *ptr2;
	SavedImage *image1, *image2;

	width = gif1->width + gif2->width;
	height = gif1->height;
	strcpy(file_name, gif1->file_name);
	strcat(file_name, ".union");

	ret = gif_open(gif, file_name, width, height, E_WRITE);
	if (e_check(ret<=0))
		goto error;
	ret = gif_put_header(gif);
	if (e_check(ret<=0))
		goto error;

	if (e_check(!(gif1&&gif1->state&&gif2&&gif2->state)))
		goto error;
	GifFile1 = (GifFileType *) gif1->handle;
	GifFile2 = (GifFileType *) gif2->handle;
	GifFile = (GifFileType *) gif->handle;

	if (DGifSlurp(GifFile1) == GIF_ERROR) {
		EGifCloseFile(GifFile1);
		GifFile1 = DGifOpenFileName(gif1->file_name, &error_code);
	}

	if (DGifSlurp(GifFile1) == GIF_ERROR)
	{
		DMSG((STDOUT,"GifFile1 read failed.\n"));
		gif_close(gif);
		return E_ERROR_INVALID_CALL;
	}

	if (DGifSlurp(GifFile2) == GIF_ERROR) {
		EGifCloseFile(GifFile2);
		GifFile2 = DGifOpenFileName(gif2->file_name, &error_code);
	}

	if (DGifSlurp(GifFile2) == GIF_ERROR)
	{
		DMSG((STDOUT,"GifFile2 read failed.\n"));
		gif_close(gif);
		return E_ERROR_INVALID_CALL;
	}

	gif1->handle = (size_t)GifFile1;
	gif2->handle = (size_t)GifFile2;

	line = (e_uint8*) malloc(width);
	count = GifFile1->ImageCount > GifFile2->ImageCount ?
															GifFile1->ImageCount :
															GifFile2->ImageCount;
	image1 = GifFile1->SavedImages, image2 = GifFile2->SavedImages;
	for (i = 0; i < count; i++) {
		ret = EGifPutImageDesc(GifFile, 0, 0, width, height, FALSE, NULL);
		if (e_check(ret == GIF_ERROR))
			goto error;

		ptr1 = image1->RasterBits;
		ptr2 = image2->RasterBits;

		for (j = 0; j < height; j++) {
			memcpy(line, ptr1, gif1->width);
			memcpy(line + gif1->width, ptr2, gif2->width);
			EGifPutLine(GifFile, (GifPixelType*) line, width);
			ptr1 += gif1->width;
			ptr2 += gif2->width;
		}

		if (i < GifFile1->ImageCount)
			image1++;
		if (i < GifFile2->ImageCount)
			image2++;
	}

	if(line) free(line);
	return E_OK;
	error:
	if(line) free(line);
	gif_close(gif);
	return E_ERROR;
}
e_int32
gif_append(gif_t * gif1, gif_t * gif2)
{
	GifFileType *GifFileOut, *Inclusion;
	bool DoTranslation;
	int error_code, ret, i;
	GifPixelType Translation[256];
	SavedImage *CopyFrom;

	e_assert(gif1&&gif1->state&&gif2&&gif2->state, E_ERROR_INVALID_HANDLER);
	GifFileOut = (GifFileType *) gif1->handle;
	Inclusion = (GifFileType *) gif2->handle;

	if (DGifSlurp(Inclusion) == GIF_ERROR) {
		EGifCloseFile(Inclusion);
		DGifOpenFileName(gif2->file_name, &error_code);
	}

	if (DGifSlurp(Inclusion) == GIF_ERROR)
	{
		DMSG((STDOUT,"Inclusion read failed.\n"));
		if (Inclusion != NULL) {
			DGifCloseFile(Inclusion);
		}
		if (GifFileOut != NULL) {
			EGifCloseFile(GifFileOut);
		}
		return E_ERROR_INVALID_CALL;
	}

	if ((DoTranslation = (GifFileOut->SColorMap != (ColorMapObject*) NULL)))
	{
		ColorMapObject *UnionMap;
		UnionMap = GifUnionColorMap(GifFileOut->SColorMap,
									Inclusion->SColorMap, Translation);
		if (UnionMap == NULL)
		{
			DMSG((STDOUT,"Inclusion failed --- global map conflict.\n"));
			if (Inclusion != NULL)
				DGifCloseFile(Inclusion);
			if (GifFileOut != NULL)
				EGifCloseFile(GifFileOut);
			return E_ERROR;
		}

		GifFreeMapObject(GifFileOut->SColorMap);
		GifFileOut->SColorMap = UnionMap;
	}

	for (CopyFrom = Inclusion->SavedImages;
			CopyFrom < Inclusion->SavedImages + Inclusion->ImageCount;
			CopyFrom++)
			{
		ret = EGifPutImageDesc(GifFileOut, 0, 0, CopyFrom->ImageDesc.Width,
								CopyFrom->ImageDesc.Height, FALSE,NULL);
		e_assert(ret != GIF_ERROR, E_ERROR);
		e_uint8* ptr = (e_uint8*) CopyFrom->RasterBits;
		for (i = 0; i < CopyFrom->ImageDesc.Height; i++) {
			EGifPutLine(GifFileOut, (GifPixelType*) ptr, CopyFrom->ImageDesc.Width);
			ptr += CopyFrom->ImageDesc.Width;
		}

	}

	return E_OK;
}

