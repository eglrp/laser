
#include "hd_jpg.h"

hdjpg::hdjpg(void)
{
	m_szPicDir = "";
}


hdjpg::~hdjpg(void)
{
	m_vecLeftImage.clear();
	m_vecRightImage.clear();
	VecPixelCols().swap(m_vecLeftImage);
	VecPixelCols().swap(m_vecRightImage);
}

void hdjpg::SetScanArea(double dHStart,double dHEnd,double dVStart,double dVEnd)
{
	m_dHStartAngle = dHStart;
	m_dHEndAngle = dHEnd;
	m_dVStartAngle = dVStart;
	m_dVEndAngle = dVEnd;
}

double hdjpg::GetCurrentHAngle()
{
	return m_dCurrentAngle;

}

void hdjpg::CaclSides()
{
		//左边 水平为 0到180度，右边为水平180到360
	if (m_dHStartAngle < 181.2 && m_dHEndAngle <=  181.2)//暂时只考虑水平角起始角小于水平终止角
	{
		m_bLeftFill = true;
		m_bRightFill = false;

	}
	else if (m_dHStartAngle < 181.2 && m_dHEndAngle > 181.2)
	{
		m_bLeftFill = true;
		m_bRightFill = true;
	}
	else if (m_dHStartAngle >= 181.2 && m_dHEndAngle > 181.2)
	{
		m_bLeftFill = false;
		m_bRightFill = true;
	}
}

void hdjpg::FillBuffer(VecRing vecEchos)
{

	//判断左边和右边的填充范围,有可能就只填充左边或者右边的，所以需要判断下
	bool bLeftFill = false; // 左部是否需要填充
	bool bRightFill = false;// 右部是否需要填充
	
	//先获取当前水平角度
	double dCurrentHAngle = GetCurrentHAngle();

	//填充左边的，获取一圈数据进行填充时，首先作如下判断
	// 1 当前水平角度是否在扫描水平角度左边范围，如果是则到2，否则不做填充
	// 2 判断一圈数据中那些是左边扫描范围的数据，如果是则填充，否则放弃，
	//填充右边时，做出同上的判断，看那些数据需要，那些数据不需要
	//----------------------------------------begin-----------------------------------
	if (m_bLeftFill && dCurrentHAngle >= m_dHStartAngle && dCurrentHAngle <= 180 && dCurrentHAngle <= m_dHEndAngle)
	{
		 bLeftFill = true; //在判断出左边需要填充后,再来根据当前水平角度来判断，当前水平角度的一圈数据是否需要保存
	}
	else
	{
		bLeftFill = false;
	}
	if (bLeftFill)
	{
		VecRing::iterator itBegin = vecEchos.begin();//从前往后遍历，同右边反过来
		VecRing::iterator itEnd = vecEchos.end();
		//水平过滤后，再通过垂直起始和终止角度来过滤每一边中对于每一圈数据的取舍
		VecPixelCol vecPixels;
		for(;itBegin != itEnd; ++itBegin)
		{
			double dAngle = itBegin->dAngle;
			if (dAngle > m_dVEndAngle)
			{
				break;
			}
			else if (dAngle < m_dVStartAngle)
			{
				continue;
			}
			int nEcho = itBegin->nRef;
			int iRGB = m_iMinRGB + (((float)nEcho - 70) / m_nEchoSpan) * (m_iMaxRGB - m_iMinRGB);//20120809(-70)
			vecPixels.push_back(iRGB);
		}// end for
		m_vecLeftImage.push_back(vecPixels);
	}
	//----------------------------------------end-----------------------------------


	//----------------------------------------begin-----------------------------------
	dCurrentHAngle += 180;
	if (m_bRightFill && dCurrentHAngle >= m_dHStartAngle && dCurrentHAngle >= 180 && dCurrentHAngle <= m_dHEndAngle)
	{
		bRightFill = true; //在判断出右边需要填充后,再来根据当前水平角度来判断，当前水平角度的一圈数据是否需要保存
	}
	else
	{
		bRightFill = false;
	}
	//填充右边的buffer
	if (bRightFill)
	{
		VecRing::reverse_iterator itBegin = vecEchos.rbegin();//从后往前遍历
		VecRing::reverse_iterator itEnd = vecEchos.rend();
		VecPixelCol vecPixels;
		for(;itBegin != itEnd; ++itBegin)//右边和左边应该返回来，每一列的点的顺序应该适合左边反过来的
		{

			int nEcho = itBegin->nRef;
			double dAngle = itBegin->dAngle;
			if (dAngle > (360 - m_dVStartAngle))
			{
				continue;
			}
			else if (dAngle < (360 - m_dVEndAngle))
			{
				break;
			}
			int iRGB = m_iMinRGB + (((float)nEcho -70) / m_nEchoSpan) * (m_iMaxRGB - m_iMinRGB);//20120809(-70)
			vecPixels.push_back(iRGB);
			
		}// end for

		m_vecRightImage.push_back(vecPixels);
	}//end if 
	//----------------------------------------end-----------------------------------
}

unsigned char * hdjpg::GetUnionBuffer()
{
	if (m_bLeftFill && m_bRightFill)//左右边都有
	{
		if (m_vecLeftImage.size() > 0 && m_vecRightImage.size() > 0)
		{
			if ((m_vecLeftImage.at(0).size() - m_vecRightImage.at(0).size()) > 1)//左右两边条带数不同，则有问题
			{
				return NULL;
			}
		}
		
	}

	int nRow = 0;
	if (m_bLeftFill && m_bRightFill)
	{
		if (m_vecLeftImage.size() > 0 && m_vecRightImage.size() > 0)
		{
			int nLeftRow = m_vecLeftImage[0].size() ;
			int nRightRow = m_vecRightImage[0].size();
			nRow = nLeftRow > nRightRow? nRightRow: nLeftRow;
			nRealCol = m_vecLeftImage.size() + m_vecRightImage.size();
		}
	}
	else if (m_bLeftFill)
	{
		if (m_vecLeftImage.size() > 0)
		{
			nRow = m_vecLeftImage[0].size() ;
			nRealCol = m_vecLeftImage.size();
		}
	}
	else if (m_bRightFill)
	{
		if (m_vecRightImage.size() > 0)
		{
			nRow = m_vecRightImage[0].size();
			nRealCol = m_vecRightImage.size();
		}
	}

	int iLeftSize = m_vecLeftImage.size();
	int iRightSize = m_vecRightImage.size();
	nRealRow = nRow;
	unsigned char * pImageBuffer = (unsigned char *)malloc(nRealCol * nRow * 3);;//深度图缓存

	for (int iCol = 0; iCol < nRealCol; ++iCol)
	{
		for (int iRow = 0; iRow < nRow ; ++iRow) //一列一列的填充,每一列有nRow个点
		{
			VecPixelCol& vec = (iCol >= iLeftSize)? m_vecRightImage[(iCol - iLeftSize)]:m_vecLeftImage[ iCol];
			int iRGB = 0;
			if (vec.size() > 0)
			{
				iRGB = vec[iRow] * 2;
			}

			if(iRGB > 255 || iRGB < 0)
			{
				*(pImageBuffer +  iCol * 3 + iRow * nRealCol * 3)  = 255;
				*(pImageBuffer +  iCol * 3 + iRow * nRealCol * 3 + 1)  = 255;
				*(pImageBuffer +  iCol * 3 + iRow * nRealCol * 3 + 2)  = 255;
			}
			else
			{
				*(pImageBuffer +  iCol * 3 + iRow * nRealCol * 3)  = iRGB;
				*(pImageBuffer +  iCol * 3 + iRow * nRealCol * 3 + 1)  = iRGB;
				*(pImageBuffer +  iCol * 3 + iRow * nRealCol * 3 + 2)  = iRGB;
			}

		}
	}
	return pImageBuffer;//外面调用的地方，记得释放该内存
}

int hdjpg::SaveImage(char *filename, unsigned char *bits, int width, int height, int depth)
{
	struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE * outfile;                 /* target file */
    JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */
    int     row_stride;             /* physical row width in image buffer */
 
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
 
    if ((outfile = fopen(filename, "wb")) == NULL) {
            fprintf(stderr, "can't open %s/n", filename);
            return -1;
    }
    jpeg_stdio_dest(&cinfo, outfile);
 
    cinfo.image_width = width;      /* image width and height, in pixels */
    cinfo.image_height = height;
    cinfo.input_components = 3;         /* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB;         /* colorspace of input image */
 
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, JPEG_QUALITY, TRUE /* limit to baseline-JPEG values */);
 
    jpeg_start_compress(&cinfo, TRUE);
 
    row_stride = width * depth; /* JSAMPLEs per row in image_buffer */
 
    while (cinfo.next_scanline < cinfo.image_height) {
            //这里我做过修改，由于jpg文件的图像是倒的，所以改了一下读的顺序
            row_pointer[0] = & bits[cinfo.next_scanline * row_stride];
       // row_pointer[0] = & bits[(cinfo.image_height - cinfo.next_scanline - 1) * row_stride];
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
 
    jpeg_finish_compress(&cinfo);
    fclose(outfile);
 
    jpeg_destroy_compress(&cinfo);
	return 0;
}

void hdjpg::SetParam(double dHStart,double dHEnd,double dVStart,double dVEnd,double dResolution)
{
	SetScanArea(dHStart,dHEnd,dVStart,dVEnd);
	CaclSides();
	m_iPoints = (m_dVEndAngle - m_dVStartAngle) / dResolution;
}

void hdjpg::ResampleBuffer(unsigned char* pBuffer, unsigned int nRows, unsigned int nCols, unsigned int nPixelSize,
	unsigned char* pDestBuffer, unsigned int nDestRows, unsigned int nDestCols)
{
	if (!pBuffer || !pDestBuffer || (0 == nRows) || (0 == nCols) || (0 == nDestRows) || (0 == nDestCols))
	{
		return;
	}

	float frRatio = (float)nRows/nDestRows;
	float fcRatio = (float)nCols/nDestCols;

	int i = 0;
	int j = 0;
	float frIndex = 0.0;
	float fcIndex = 0.0;

	unsigned char* pSrc = pBuffer;
	unsigned char* pDest = pDestBuffer;

	for (i = 0; i<nDestRows; i++)
	{
		for (fcIndex = 0.0, j = 0; j<nDestCols; j++)
		{
			memcpy((pDest + j * nPixelSize), (pSrc + (unsigned int)fcIndex * nPixelSize), nPixelSize);
			fcIndex += fcRatio;
		}

		frIndex += frRatio;
		pSrc = pBuffer + (unsigned int)frIndex * nCols * nPixelSize;
		pDest = pDestBuffer + i * nDestCols * nPixelSize;
	}
}

void hdjpg::Clear()
{
	m_vecLeftImage.clear();
	VecPixelCols().swap(m_vecLeftImage);
	m_vecRightImage.clear();
	VecPixelCols().swap(m_vecRightImage);
}
void hdjpg::GeneraPic()
{
	//生成灰度图
	unsigned char * pImageBuffer = GetUnionBuffer();
	double dReference = (m_dHEndAngle - m_dHStartAngle) / (m_dVEndAngle - m_dVStartAngle);//比例调整（20130118）//nRealRow * 4 * (m_dHEndAngle - m_dHStartAngle) /360;
	//int nReference = (int)dReference;
	if (nRealCol > 0 && nRealRow > 0)
	{
		if (1.0 * nRealCol /nRealRow >  dReference)
		{
			double dRefCol = nRealRow * dReference;
			int nRefCol = (int)dRefCol;
			unsigned char* image_buffer2 = (unsigned char *)malloc(nRefCol * nRealRow * 3);
			ResampleBuffer(pImageBuffer,nRealRow,nRealCol,3,image_buffer2,nRealRow,nRefCol);
			SaveImage(m_szPicDir, image_buffer2, nRefCol, nRealRow, 3);
			delete [] image_buffer2;
		}
		else
		{
			double dRefRow = nRealCol / dReference;
			int nRefRow = (int)dRefRow;
			unsigned char* image_buffer2 = (unsigned char *)malloc(nRealCol * nRefRow * 3);
			ResampleBuffer(pImageBuffer,nRealRow,nRealCol,3,image_buffer2,nRefRow,nRealCol);
			SaveImage(m_szPicDir, image_buffer2,nRealCol,nRefRow,3);
			delete [] image_buffer2;
		}
	}
	//delete m_pJpgCvt;
	Clear();
	//	Sleep(3000);
	delete [] pImageBuffer;
}

void hdjpg::SetPicDir(char* szPicDir)
{
	m_szPicDir = szPicDir;
}
