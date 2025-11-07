/**
 ** read a BMP file into a Texture:
 **/

#include <stdio.h>
#include <math.h>

#define BMP_MAGIC_NUMBER	0x4d42
#ifndef BI_RGB
#define BI_RGB			0
#define BI_RLE8			1
#define BI_RLE4			2
#endif



int
ReadInt( FILE *fp )
{
	const unsigned char b0 = fgetc( fp );
	const unsigned char b1 = fgetc( fp );
	const unsigned char b2 = fgetc( fp );
	const unsigned char b3 = fgetc( fp );
	return ( b3 << 24 )  |  ( b2 << 16 )  |  ( b1 << 8 )  |  b0;
}


short
ReadShort( FILE *fp )
{
	const unsigned char b0 = fgetc( fp );
	const unsigned char b1 = fgetc( fp );
	return ( b1 << 8 )  |  b0;
}


// bmp file header:
struct bmfh
{
	short bfType;		// BMP_MAGIC_NUMBER = "BM"
	int bfSize;		// size of this file in bytes
	short bfReserved1;
	short bfReserved2;
	int bfOffBytes;		// # bytes to get to the start of the per-pixel data
} FileHeader;

// bmp info header:
struct bmih
{
	int biSize;		// info header size, should be 40
	int biWidth;		// image width
	int biHeight;		// image height
	short biPlanes;		// #color planes, should be 1
	short biBitCount;	// #bits/pixel, should be 1, 4, 8, 16, 24, 32
	int biCompression;	// BI_RGB, BI_RLE4, BI_RLE8
	int biSizeImage;
	int biXPixelsPerMeter;
	int biYPixelsPerMeter;
	int biClrUsed;		// # colors in the palette
	int biClrImportant;
} InfoHeader;



// read a BMP file into a Texture:

int
ifloor( float val )
{
	return (int)( floor(val) );
}

int
iceil( float val )
{
	return (int)( ceil(val) );
}


unsigned char *
BmpToTexture( char *filename, int *width, int *height )
{
	FILE* fp;
#ifdef _WIN32
        errno_t err = fopen_s( &fp, filename, "rb" );
        if( err != 0 )
#else
	fp = fopen( filename, "rb" );
	if( fp == NULL )
#endif
	{
		fprintf( stderr, "Cannot open Bmp file '%s'\n", filename );
		return NULL;
	}

	FileHeader.bfType = ReadShort( fp );


	// if bfType is not BMP_MAGIC_NUMBER, the file is not a bmp:

	if( FileHeader.bfType != BMP_MAGIC_NUMBER )
	{
		fprintf( stderr, "File '%s' is not a BMP file -- it's type is: 0x%0x\n", filename, FileHeader.bfType );
		fclose( fp );
		return NULL;
	}

	FileHeader.bfSize = ReadInt( fp );
	FileHeader.bfReserved1 = ReadShort( fp );
	FileHeader.bfReserved2 = ReadShort( fp );
	FileHeader.bfOffBytes = ReadInt( fp );
	InfoHeader.biSize = ReadInt( fp );
	InfoHeader.biWidth = ReadInt( fp );
	InfoHeader.biHeight = ReadInt( fp );

	// the horizontal and vertical dimensions:
	int nums = InfoHeader.biWidth;
	int numt = InfoHeader.biHeight;

	InfoHeader.biPlanes = ReadShort( fp );
	InfoHeader.biBitCount = ReadShort( fp );
	fprintf(stderr, "Image file '%s' has pixel dimensions %dx%d and %d bits/pixel\n",
			filename, nums, numt, InfoHeader.biBitCount);

	InfoHeader.biCompression = ReadInt( fp );
	InfoHeader.biSizeImage = ReadInt( fp );
	InfoHeader.biXPixelsPerMeter = ReadInt( fp );
	InfoHeader.biYPixelsPerMeter = ReadInt( fp );
	InfoHeader.biClrUsed = ReadInt( fp );
	InfoHeader.biClrImportant = ReadInt( fp );

	// pixels will be stored bottom-to-top, left-to-right:
	int numcomps = InfoHeader.biBitCount / 8;

	unsigned char *texture = new unsigned char[ 3 * nums * numt ];
	if( texture == NULL )
	{
		fprintf( stderr, "Cannot allocate the texture array!\n" );
		fclose( fp );
		return NULL;
	}

	// extra padding bytes:

	int requiredRowSizeInWords = ifloor( ( (float)(InfoHeader.biBitCount*InfoHeader.biWidth) + 31.f )  /  32.f );
	int requiredRowSizeInBytes = 4 * requiredRowSizeInWords;
	int rowSizeInBytes = numcomps * InfoHeader.biWidth;
	int numPadBytes = requiredRowSizeInBytes - rowSizeInBytes;

	unsigned char uc;	// to read unneeded bytes

	// we can handle 24 bits of direct color:
	if( InfoHeader.biBitCount == 24 )
	{
		// 24-bit does not want to see the compression bits set:

		if (InfoHeader.biCompression != 0)
		{
			fprintf(stderr, "Wrong type of image compression: %d\n", InfoHeader.biCompression);
			fclose(fp);
			return NULL;
		}

		rewind(fp);
		fseek(fp, FileHeader.bfOffBytes, SEEK_SET);

		if( numcomps == 3  ||  numcomps == 4 )
		{
			unsigned char *tp = &texture[0];	// at the start
			for (int t = 0; t < numt; t++)
			{
				for( int s = 0; s < nums; s++ )
				{
					if( numcomps == 4 )
						uc = fgetc( fp );					// a
					unsigned char b = fgetc( fp );			// b
					unsigned char g = fgetc( fp );			// g
					unsigned char r = fgetc( fp );			// r
					*tp++ = r;
					*tp++ = g;
					*tp++ = b;
				}
				for( int e = 0; e < numPadBytes; e++ )
				{
					uc = fgetc( fp );
					//if( uc != 0 )		fprintf( stderr, "%4d: %3d ", t, uc );
				}
			}
		}
	}

	// we can also handle 8 bits of indirect color:
	if (InfoHeader.biBitCount == 8 && InfoHeader.biClrUsed == 256)
	{
		// 8-bit does not want to see the compression bits set:

		if (InfoHeader.biCompression != 0)
		{
			fprintf(stderr, "Wrong type of image compression: %d\n", InfoHeader.biCompression);
			fclose(fp);
			return NULL;
		}

		struct rgba32
		{
			unsigned char r, g, b, a;
		};

		struct rgba32* colorTable = new struct rgba32[InfoHeader.biClrUsed];

		rewind(fp);
		fseek(fp, sizeof(struct bmfh) + InfoHeader.biSize - 2, SEEK_SET);
		for (int c = 0; c < InfoHeader.biClrUsed; c++)
		{
			colorTable[c].b = fgetc(fp);
			colorTable[c].g = fgetc(fp);
			colorTable[c].r = fgetc(fp);
			colorTable[c].a = fgetc(fp);
			//if (Verbose)	fprintf(stderr, "%4d:\t0x%02x\t0x%02x\t0x%02x\t0x%02x\n",
				//c, colorTable[c].r, colorTable[c].g, colorTable[c].b, colorTable[c].a);
		}

		rewind(fp);
		fseek(fp, FileHeader.bfOffBytes, SEEK_SET);

		if( numcomps == 1 )
		{
			unsigned char *tp = &texture[0];	// at the start
			for (int t = 0; t < numt; t++)
			{
				for (int s = 0; s < nums; s++)
				{
					int index = fgetc(fp);
					unsigned char r = colorTable[index].r;	// r
					unsigned char g = colorTable[index].g;	// g
					unsigned char b = colorTable[index].b;	// b
					*tp++ = r;
					*tp++ = g;
					*tp++ = b;
				}
				for (int e = 0; e < numPadBytes; e++)
				{
					uc = fgetc(fp);
					//if( uc != 0 )		fprintf( stderr, "%4d: %3d ", t, uc );
				}
			}
		}
		delete[] colorTable;
	}

	// we can handle 32 bits of direct color:
	if (InfoHeader.biBitCount == 32)
	{
		// 32-bit doesn't mind if the compression bits are set -- we just ignore them:
		rewind( fp );
		fseek( fp, FileHeader.bfOffBytes, SEEK_SET );

		if( numcomps == 4 )
		{
			unsigned char *tp = &texture[0];	// at the start
			for (int t = 0; t < numt; t++)
			{
				for (int s = 0; s < nums; s++)
				{
					unsigned char b = fgetc(fp);		// b
					unsigned char g = fgetc(fp);		// g
					unsigned char r = fgetc(fp);		// r
					uc = fgetc(fp);				// a
					*tp++ = r;
					*tp++ = g;
					*tp++ = b;
				}
				for (int e = 0; e < numPadBytes; e++)
				{
					uc = fgetc(fp);
					//if( uc != 0 )		fprintf( stderr, "%4d: %3d ", t, uc );
				}
			}
		}
	}

	fclose( fp );
	*width  = nums;
	*height = numt;
	return texture;
}
