#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifndef F_PI
#define F_PI		((float)(M_PI))
#define F_2_PI		((float)(2.f*F_PI))
#define F_PI_2		((float)(F_PI/2.f))
#endif

inline
void
DrawSphLatLng( float radius, float lat, float lng )
{
	// lat is in radians between -F_PI_2 and +F_PI_2
	// lng is in radians between -F_PI and +F_PI
	float xz =  cosf(lat);
	float x = xz * sinf(lng);
	float y = sinf(lat);
	float z = xz * cosf(lng);
	float nx = x;		// for a *sphere only*, the normal is the unitized position
	float ny = y;		// for a *sphere only*, the normal is the unitized position
	float nz = z;		// for a *sphere only*, the normal is the unitized position
	float s = ( lng + F_PI )   / F_2_PI;
	float t = ( lat + F_PI_2 ) / F_PI;
	//glColor3f( 1.f, 0.5f, 0.f );
	glTexCoord2f( s, t );
	glNormal3f( nx, ny, nz );
	glVertex3f( x*radius, y*radius, z*radius );
}


void
OsuSphere( float radius, int slices, int stacks )
{
	// sanity check:
	radius = (float)fabs(radius);
	if( slices < 4 )		slices = 4;
	if( stacks < 4 )		stacks = 4;


	glBegin( GL_TRIANGLES );

	// south pole:
	{
		int istack = 0;
		float north = -F_PI_2 + F_PI * (float)(istack + 1) / (float)stacks;
		float south = -F_PI_2 + F_PI * (float)(istack + 0) / (float)stacks;
		for (int islice = 0; islice < slices; islice++)
		{
			float west = -F_PI + F_2_PI * (float)(islice + 0) / (float)slices;
			float east = -F_PI + F_2_PI * (float)(islice + 1) / (float)slices;

			DrawSphLatLng( radius, south, .5f * (east + west));
			DrawSphLatLng( radius, north, east);
			DrawSphLatLng( radius, north, west);
		}
	}

	// north pole:
	{
		int istack = stacks - 1;
		float north = -F_PI_2 + F_PI * (float)(istack + 1) / (float)stacks;
		float south = -F_PI_2 + F_PI * (float)(istack + 0) / (float)stacks;
		for (int islice = 0; islice < slices; islice++)
		{
			float west = -F_PI + F_2_PI * (float)(islice + 0) / (float)slices;
			float east = -F_PI + F_2_PI * (float)(islice + 1) / (float)slices;

			DrawSphLatLng( radius, north, .5f*(east + west) );
			DrawSphLatLng( radius, south, west );
			DrawSphLatLng( radius, south, east );
		}
	}

	// all the bands in between:
	for (int istack = 1; istack < stacks-1; istack++)
	{
		float north = -F_PI_2 + F_PI * (float)(istack + 1) / (float)stacks;
		float south = -F_PI_2 + F_PI * (float)(istack + 0) / (float)stacks;
		for (int islice = 0; islice < slices; islice++)
		{
			float west = -F_PI + F_2_PI * (float)(islice + 0) / (float)slices;
			float east = -F_PI + F_2_PI * (float)(islice + 1) / (float)slices;

			DrawSphLatLng( radius, north, west );
			DrawSphLatLng( radius, south, west );
			DrawSphLatLng( radius, north, east );

			DrawSphLatLng( radius, north, east );
			DrawSphLatLng( radius, south, west );
			DrawSphLatLng( radius, south, east );
		}
	}

	glEnd( );
}
