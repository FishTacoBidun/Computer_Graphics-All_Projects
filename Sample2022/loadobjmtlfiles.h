#ifndef LOADOBJFILE_H
#define LOADOBJFILE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <GL/gl.h>
#include <vector>


#define OBJDELIMS		" \t"

const void  *	NULLPTR =     (void *)0;

const char *	DELIMS = " \t";

struct Vertex
{
	float x, y, z;
};

struct Normal
{
	float nx, ny, nz;
};

struct TextureCoord
{
	float s, t, p;
};

struct face
{
	int v, n, t;
};


GLuint	LoadObjFile( char * );

void	Cross( float [3], float [3], float [3] );
char *	ReadRestOfLine( FILE * );
void	ReadObjVTN( char *, int *, int *, int * );
float	Unit( float [3] );
float	Unit( float [3], float [3] );

int	Readline( FILE *, char * );
float * Array3( float, float, float );
float * Array3( float * );

int	FindMtlName( char * );
void	SetOpenglMtlProperties( );
char *	GetSuffix( char * );


unsigned char * BmpToTexture( char *, int *, int * );


#endif		// #ifndef
