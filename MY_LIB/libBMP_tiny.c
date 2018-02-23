/*
 * libBMP_tiny.c
 *
 *  Created on: 20 марта 2015 г.
 *      Author: dmitry
 */
#include "libBMP_tiny.h"
#include "SSD1289.h"
#include "ff.h"

#define USE_BACKGROUND_COLOR

#define IMG_BUF_LEN			552				//256

#define CONVERT_TO_565(R, G, B) 	( ( (R << 8) & 0xF800) | ((G << 3) & 0x7E0) | ((B >> 3) & 0x1F) )

BITMAPFILEHEADER file_header;

FIL file;
FRESULT res;
UINT cnt;

char BMP_biBitCount = 0;

bool read_bitmap_header(void)
{
	f_read (&file, &file_header, sizeof(BITMAPFILEHEADER), &cnt);

	if(file_header.bfType == 0x4D42) {
//		LCD_PutString(10, nextLinePos(), "image is BMP LE");
		return true;
	}
	else if(file_header.bfType == 0x424D) {
//		LCD_PutString(10, nextLinePos(), "image is BMP BE");
		return true;
	}
	else
		LCD_PutString(10, nextLinePos(), "image is not BMP!!!");
		return false;
}
/**
  * @brief  This function drawing Full Screen image from File Sistem
  * @param  *fname(const char): file name in file sistem
  * @retval Bool: ok - 0, error - 1
  */
bool LCD_PutBMP_FS(const char *fname)
{
	bool isBMP = 1;
	uint32_t mindex;
	uint16_t img_buff[IMG_BUF_LEN];

	res = f_open( &file, fname, FA_READ );
	isBMP = read_bitmap_header();

	if(!res && isBMP) {
	  res = f_lseek( &file, file_header.bfOffBits );
	  if(!res) {
		  LCD_SetLORightDown_Win(239, 319, 240, 320);
		  LCD_WriteReg(LCD_REG_17, 0x6048);
		  LCD_WriteRAM_Prepare();

		  do {
			  res = f_read (&file, &img_buff[0], IMG_BUF_LEN*2, &cnt);
			  for(mindex = 0; mindex < cnt/2 ; mindex++) {
				  LCD_WriteRAM( img_buff[mindex] );
			  }
		  }while(res == 0 && cnt == IMG_BUF_LEN*2);
		  LCD_WriteReg(LCD_REG_17, 0x6028);
		  LCD_WriteReg(0x0044,0xEF00); // 239
		  LCD_WriteReg(0x0045,0x0000); // 0
		  f_close(&file);
		  return 0;
	  }
	  else
	  	  return 1;
	}
	else
		return 1;
}

/*	Фунцйия считывающая заголовок BMP изображения,
 * получат данные о смещении данных, высоте и ширине,
 * количестве бит на пиксель, формат данных (LE/BE)
 */
bool ReadBitmapHeader(FIL *fp, SIZE_IMAGE *size)
{
	UINT cnt;
	//SIZE_IMAGE size={0,0};
	BITMAPINFOHEADER info_header;

	f_read (fp, &file_header, sizeof(BITMAPFILEHEADER), &cnt);

	if(file_header.bfType == 0x4D42) {
		f_read (fp, &info_header, sizeof(BITMAPINFOHEADER), &cnt);
		size->Height = info_header.biHeight;
		size->Width = info_header.biWidth;

		BMP_biBitCount = info_header.biBitCount;
		return 0;
	}
	else if(file_header.bfType == 0x424D) {
		LCD_PutString(10, nextLinePos(), "image is not LE!!!");
		return 1;
	}
	else
		LCD_PutString(10, nextLinePos(), "image is not BMP!!!");
		return 1;
}

/*
 *  Сначала считывает данные изображения в буфер, для многократного прорисовывания из буфера.
 *  Пропадает необходимость постоянно считывать один и тот же файл
 */

bool ReadIconToBuff(const char *fname, uint16_t *BitmapBuff, SIZE_IMAGE *size )
{
	FIL file;
	UINT cnt;
	FRESULT res;
	bool readimg;

	res = f_open( &file, fname, FA_READ );

	readimg = ReadBitmapHeader(&file, size);

	if(res || !readimg)
		return false;

	res = f_lseek(&file, file_header.bfOffBits);
	if(res)
		return false;

	res = f_read (&file, BitmapBuff, SIZE_ICON_BUFF*2 , &cnt);

	f_close(&file);
	return true;
}

/*
 *  Вывод BMP изображения форматов RGB 565 (16 бит) и 888 (24 бита).
 *  Изображение должно быть равно разрешению экрана или меньше его
 *  Картинка рисуется по центру экрана
 */

bool ShowImageSize(const char *fname, uint8_t Xpos, uint16_t Ypos)
{
	  SIZE_IMAGE size;
	  uint8_t img_buff[IMG_BUF_LEN];
	  UINT cnt;
	  FRESULT res;
	  uint32_t mindex;
	  res = f_open( &file, fname, FA_READ );

	  if( res || ReadBitmapHeader(&file, &size) )
		  return 1;

	  if( BMP_biBitCount != 16 && BMP_biBitCount != 24 )
		  return 1;

	  res = f_lseek(&file, file_header.bfOffBits);
	  if( res )
		  return 1;

	  if( size.Height == 240 /*|| size.Height == 320*/ )
	  {
		  LCD_SetLORightDown_Win(239, 319, 240, 320);
		  LCD_WriteReg(LCD_REG_17, 0x6048); // AM=0; ID[1:0]=11
	  }
	  else
	  {
		  LCD_Clear(LCD_BackColor());
		  LCD_SetPORightUp_Win( (240/2)-size.Height/2, (320/2)+size.Width/2, size.Width, size.Height );
		  LCD_WriteReg(LCD_REG_17, 0x6810); // AM=0; ID[1:0]=01
	  }

	  LCD_WriteRAM_Prepare();

	  if ( BMP_biBitCount == 16 )
	  {
		  do {
				  res = f_read (&file, &img_buff[0], IMG_BUF_LEN, &cnt);
				  for(mindex = 0; mindex < cnt ; mindex+=2) {

#if defined(USE_BACKGROUND_COLOR)
					  if( ( img_buff[mindex] == 0xFF ) && ( img_buff[mindex+1] == 0xFF ) )
						  LCD_WriteRAM( LCD_BackColor() );
					  else
						  LCD_WriteRAM( (uint16_t)( img_buff[mindex] | (img_buff[mindex+1]) << 8 ) );
#else
					  LCD_WriteRAM( (uint16_t)( img_buff[mindex] | (img_buff[mindex+1]) << 8 ) );
#endif

				  }
			  }while(res == 0 && cnt == IMG_BUF_LEN);
	  }
	  else /*if( BMP_biBitCount == 24 )*/
	  {
		  do {
				  res = f_read (&file, &img_buff[0], IMG_BUF_LEN, &cnt);
				  for(mindex = 0; mindex < cnt ; mindex+=3) {
					  //									(	RRRRR			,	GGGGGG			,	BBBBB		)
					  LCD_WriteRAM( (uint16_t)CONVERT_TO_565(img_buff[mindex+2], img_buff[mindex+1], img_buff[mindex]) );
				  }
			  }while(res == 0 && cnt == IMG_BUF_LEN);
	  }

	  LCD_WriteReg(LCD_REG_17, 0x6830);
	  LCD_WriteReg(0x0044,0xEF00); // Horizontal position = 239
	  LCD_WriteReg(0x0045,0x0000); // start positions = 0

	  f_close(&file);

	  return 0;
}

