#include "loadobjmtlfiles.h"

#define HANDLE_MTL_FILES

FILE* FpMtl;;
std::vector<GLuint>	DisplayLists(50);


GLuint
LoadObjMtlFiles( char *name )
{
	char *cmd;		// the command string
	char *str;		// argument string

	std::vector <struct Vertex> Vertices(10000);
	std::vector <struct Normal> Normals(10000);
	std::vector <struct TextureCoord> TextureCoords(10000);

	Vertices.clear();
	Normals.clear();
	TextureCoords.clear();
	DisplayLists.clear();
	FpMtl = NULL;
	struct Vertex sv;
	struct Normal sn;
	struct TextureCoord st;


	// open the input file:

	FILE *fp;
#ifdef _WIN32
        errno_t err = fopen_s( &fp, name, "r" );
    	if( err != 0 )
#else
	fp = fopen( str, "r" );
	if( fp == NULL )
#endif
	{
		fprintf( stderr, "Cannot open .obj file '%s'\n", name );
		return 1;
	}


	float xmin = 1.e+37f;
	float ymin = 1.e+37f;
	float zmin = 1.e+37f;
	float xmax = -xmin;
	float ymax = -ymin;
	float zmax = -zmin;

	int numTotalTriangles = 0;

	GLuint currentDL = glGenLists( 1 );
	glNewList( currentDL, GL_COMPILE );

	DisplayLists.push_back( currentDL );

	glBegin( GL_TRIANGLES );

	for( ; ; )
	{
		char *line = ReadRestOfLine( fp );
		if( line == NULL )
			break;


		// skip this line if it is a comment:

		if( line[0] == '#' )
			continue;


		// skip this line if it is something we don't feel like handling today:

		if( line[0] == 'g' )
			continue;

		if( line[0] == 's' )
			continue;

		// get the command string:

		cmd = strtok( line, OBJDELIMS );


		// skip this line if it is empty:

		if( cmd == NULL )
			continue;


#ifdef HANDLE_MTL_FILES
		if( strcmp( cmd, "mtllib" )  ==  0 )
		{
			str = strtok( NULL, OBJDELIMS );
			//fprintf( stderr, "Material file name = '%s'\n", str );

			if( FpMtl != NULL )
				fclose( FpMtl );

#ifdef _WIN32
        	errno_t err = fopen_s( &FpMtl, str, "r" );
        	if( err != 0 )
#else
			FpMtl = fopen( str, "r" );
			if( FpMtl == NULL )
#endif
			{
				fprintf( stderr, "Cannot open mtl filename '%s'\n", str );
				FpMtl = NULL;
			}
			else
			{
				fprintf(stderr, "Opened mtl filename '%s'\n", str);

				glEnd();
				glDisable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, 0);

				glBegin(GL_TRIANGLES);
				continue;
			}
		}


		if( strcmp( cmd, "usemtl" )  ==  0 )
		{
			if (FpMtl == NULL)
				continue;		// there is no open mtl file
			
			str = strtok(NULL, OBJDELIMS);
			int status = FindMtlName( str );
			if( status == -1 )
			{
				fprintf( stderr, "Could not find a material named '%s'\n", str );
			}
			else
			{
				glEnd();

				glDisable( GL_TEXTURE_2D );
				glBindTexture(GL_TEXTURE_2D, 0);

				// the file pointer was left ready to read the next line in the mtl file
				SetOpenglMtlProperties( );		// set all the opengl stuff

				glBegin(GL_TRIANGLES);
			}
			continue;

		}
#endif

		if( strcmp( cmd, "v" )  ==  0 )
		{
			str = strtok( NULL, OBJDELIMS );
			sv.x = (float)atof(str);

			str = strtok( NULL, OBJDELIMS );
			sv.y = (float)atof(str);

			str = strtok( NULL, OBJDELIMS );
			sv.z = (float)atof(str);

			Vertices.push_back( sv );

			if( sv.x < xmin )	xmin = sv.x;
			if( sv.x > xmax )	xmax = sv.x;
			if( sv.y < ymin )	ymin = sv.y;
			if( sv.y > ymax )	ymax = sv.y;
			if( sv.z < zmin )	zmin = sv.z;
			if( sv.z > zmax )	zmax = sv.z;
			continue;
		}


		if( strcmp( cmd, "vn" )  ==  0 )
		{
			str = strtok( NULL, OBJDELIMS );
			sn.nx = (float)atof( str );

			str = strtok( NULL, OBJDELIMS );
			sn.ny = (float)atof( str );

			str = strtok( NULL, OBJDELIMS );
			sn.nz = (float)atof( str );

			Normals.push_back( sn );
			continue;
		}


		if( strcmp( cmd, "vt" )  ==  0 )
		{
			st.s = st.t = st.p = 0.;

			str = strtok( NULL, OBJDELIMS );
			st.s = (float)atof( str );

			str = strtok( NULL, OBJDELIMS );
			if( str != NULLPTR )
				st.t = (float)atof( str );

			str = strtok( NULL, OBJDELIMS );
			if( str != NULLPTR )
				st.p = (float)atof( str );

			TextureCoords.push_back( st );
			continue;
		}


		if( strcmp( cmd, "f" )  ==  0 )
		{
			struct face vertices[10];
			for( int i = 0; i < 10; i++ )
			{
				vertices[i].v = 0;
				vertices[i].n = 0;
				vertices[i].t = 0;
			}

			int sizev = (int)Vertices.size();
			int sizen = (int)Normals.size();
			int sizet = (int)TextureCoords.size();

			int numVertices = 0;
			bool valid = true;
			int vtx = 0;
			char *str;
			while( ( str = strtok( NULL, OBJDELIMS ) )  !=  NULL )
			{
				int v=0, n=0, t=0;
				ReadObjVTN( str, &v, &t, &n );

				// if v, n, or t are negative, they are wrt the end of their respective list:

				if( v < 0 )
					v += ( sizev + 1 );

				if( n < 0 )
					n += ( sizen + 1 );

				if( t < 0 )
					t += ( sizet + 1 );


				// be sure we are not out-of-bounds (<vector> will abort):

				if( t > sizet )
				{
					if( t != 0 )
						fprintf( stderr, "Read texture coord %d, but only have %d so far\n", t, sizet );
					t = 0;
				}

				if( n > sizen )
				{
					if( n != 0 )
						fprintf( stderr, "Read normal %d, but only have %d so far\n", n, sizen );
					n = 0;
				}

				if( v > sizev )
				{
					if( v != 0 )
						fprintf( stderr, "Read vertex coord %d, but only have %d so far\n", v, sizev );
					v = 0;
					valid = false;
				}

				vertices[vtx].v = v;
				vertices[vtx].n = n;
				vertices[vtx].t = t;
				vtx++;

				if( vtx >= 10 )
					break;

				numVertices++;
			}


			// if vertices are invalid, don't draw anything this time:

			if( ! valid )
				continue;

			if( numVertices < 3 )
				continue;


			// list the vertices:

			int numTriangles = numVertices - 2;

			for( int it = 0; it < numTriangles; it++ )
			{
				int vv[3];
				vv[0] = 0;
				vv[1] = it + 1;
				vv[2] = it + 2;

				// get the planar normal, in case vertex normals are not defined:

				struct Vertex *v0 = &Vertices[ vertices[ vv[0] ].v - 1 ];
				struct Vertex *v1 = &Vertices[ vertices[ vv[1] ].v - 1 ];
				struct Vertex *v2 = &Vertices[ vertices[ vv[2] ].v - 1 ];

				float v01[3], v02[3], norm[3];
				v01[0] = v1->x - v0->x;
				v01[1] = v1->y - v0->y;
				v01[2] = v1->z - v0->z;
				v02[0] = v2->x - v0->x;
				v02[1] = v2->y - v0->y;
				v02[2] = v2->z - v0->z;
				Cross( v01, v02, norm );
				Unit( norm, norm );
				glNormal3fv( norm );

				for( int vtx = 0; vtx < 3 ; vtx++ )
				{
					if( vertices[ vv[vtx] ].t != 0 )
					{
						struct TextureCoord *tp = &TextureCoords[ vertices[ vv[vtx] ].t - 1 ];
						glTexCoord2f( tp->s, tp->t );
					}

					if( vertices[ vv[vtx] ].n != 0 )
					{
						struct Normal *np = &Normals[ vertices[ vv[vtx] ].n - 1 ];
						glNormal3f( np->nx, np->ny, np->nz );
					}

					struct Vertex *vp = &Vertices[ vertices[ vv[vtx] ].v - 1 ];
					glVertex3f( vp->x, vp->y, vp->z );
				}
				numTotalTriangles++;
			}
			continue;
		}


		if( strcmp( cmd, "s" )  ==  0 )
		{
			continue;
		}

	}
	fclose( fp );

	glEnd();
	glEndList( );				// end the current display list

	// create the master dl:

	GLuint masterDL = glGenLists( 1 );
	glNewList( masterDL, GL_COMPILE );
	for (int i = 0; i < (int)DisplayLists.size(); i++)
	{
		glCallList( DisplayLists[i] );
	}
	glEndList( );

	// print the summary:

	fprintf(stderr, "OBJ file '%s' read.\n", name);
	fprintf(stderr, "\t%d total triangles\n", numTotalTriangles);
	fprintf(stderr, "\trange: [%8.3f,%8.3f,%8.3f] -> [%8.3f,%8.3f,%8.3f]\n",
		xmin, ymin, zmin, xmax, ymax, zmax);
	fprintf(stderr, "\tcenter = (%8.3f,%8.3f,%8.3f)\n",
		(xmin + xmax) / 2., (ymin + ymax) / 2., (zmin + zmax) / 2.);
	fprintf(stderr, "\tspan = (%8.3f,%8.3f,%8.3f)\n",
		xmax - xmin, ymax - ymin, zmax - zmin);

	return masterDL;
}


#ifdef NOTDEF
void
Cross( float v1[3], float v2[3], float vout[3] )
{
	float tmp[3];

	tmp[0] = v1[1]*v2[2] - v2[1]*v1[2];
	tmp[1] = v2[0]*v1[2] - v1[0]*v2[2];
	tmp[2] = v1[0]*v2[1] - v2[0]*v1[1];

	vout[0] = tmp[0];
	vout[1] = tmp[1];
	vout[2] = tmp[2];
}



float
Unit( float v[3] )
{
	float dist;

	dist = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if( dist > 0.0 )
	{
		dist = (float)sqrt( dist );
		v[0] /= dist;
		v[1] /= dist;
		v[2] /= dist;
	}

	return dist;
}



float
Unit( float vin[3], float vout[3] )
{
	float dist;

	dist = vin[0]*vin[0] + vin[1]*vin[1] + vin[2]*vin[2];

	if( dist > 0.0 )
	{
		dist = (float)sqrt( dist );
		vout[0] = vin[0] / dist;
		vout[1] = vin[1] / dist;
		vout[2] = vin[2] / dist;
	}
	else
	{
		vout[0] = vin[0];
		vout[1] = vin[1];
		vout[2] = vin[2];
	}

	return dist;
}
#endif

char *
ReadRestOfLine( FILE *fp )
{
	static char *line;
	std::vector<char> tmp(1000);
	tmp.clear();

	for( ; ; )
	{
		int c = getc( fp );

		if( c == EOF  &&  tmp.size() == 0 )
		{
			return NULL;
		}

		if( c == EOF  ||  c == '\n' )
		{
			delete [] line;
			line = new char [ tmp.size()+1 ];
			for( int i = 0; i < (int)tmp.size(); i++ )
			{
				line[i] = tmp[i];
			}
			line[ tmp.size() ] = '\0';	// terminating null
			return line;
		}
		else
		{
			tmp.push_back( c );
		}
	}

	return (char *) "";
}


void
ReadObjVTN( char *str, int *v, int *t, int *n )
{
	// can be one of v, v//n, v/t, v/t/n:

	if( strstr( str, "//") )				// v//n
	{
		*t = 0;
		int dummy = sscanf( str, "%d//%d", v, n );
		return;
	}
	else if( sscanf( str, "%d/%d/%d", v, t, n ) == 3 )	// v/t/n
	{
		return;
	}
	else
	{
		*n = 0;
		if( sscanf( str, "%d/%d", v, t ) == 2 )		// v/t
		{
			return;
		}
		else						// v
		{
			*n = *t = 0;
			int dummy = sscanf( str, "%d", v );
		}
	}
}


int
Readline( FILE *fp, char *line )
{
	char *cp = line;
	int c;
	int numChars;

	while( ( c = fgetc(fp) ) != '\n'  &&  c != EOF )
	{
		if( c == '\r' )
		{
			continue;
		}
		//fprintf( stderr, "c = '%c' (0x%02x)\n", c, c );
		*cp++ = c;
	}
	*cp = '\0';

	if( c == EOF )
		return EOF;

	// reject an empty line:

	numChars = 0;
	char cc;
	for( char *cp = line; ( cc = *cp ) != '\0'; cp++ )
	{
		if( cc != '\t'  &&  cc != ' ' )
			numChars++;
	}
	if( numChars == 0 )
	{
		line[0] = '\0';
	}

	return c;	// the character that caused the while-loop to terminate
}

#ifdef HANDLE_MTL_FILES

int
FindMtlName( char *mtlName )
{
	if( FpMtl == NULL )
	{
		fprintf( stderr, "Cannot find material '%s' -- no mtl file is currently open!\n", mtlName );
		return -1;
	}

	char line[256];
	int c;

	rewind( FpMtl );
	while( ( c = Readline( FpMtl, line) ) != EOF )
	{
		if( line[0] == '\0' )
			continue;

		//fprintf( stderr, "LINE = '%s'\n", line );

		char * tok = strtok( line, DELIMS );		// first token is the variable name
		char cc;
		for( char *cp = tok; ( cc = *cp ) != '\0'; cp++ )
		{
			if( isupper(cc) )
				*cp = tolower( cc );
		}
		//fprintf(stderr, "TOK = '%s'\n", tok);

		// if find "newmtl", see if it is the material we are looking for:
		if( strcmp( tok, "newmtl" ) == 0 )
		{
			tok = strtok( NULL, DELIMS );		// next token is the material name
			//fprintf(stderr, "'%s' material found\n", tok);

			if( strcmp( tok, mtlName ) != 0 )
				continue;

			fprintf( stderr, "Found the material: '%s'\n", mtlName );
			// leave file pointer pointing to the next line in the mtl file:
			return 0;
		}
	}
	return -1;
}

void
SetOpenglMtlProperties( )
{
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);


	char line[256];
	int c;
	while( ( c = Readline( FpMtl, line) ) != EOF )
	{
		if( c == EOF )
			break;

		if( line[0] == '\0' )
			continue;


		char * tok = strtok( line, DELIMS );			// first token is the variable name
		char cc;
		for( char *cp = tok; ( cc = *cp ) != '\0'; cp++ )
		{
			if( isupper(cc) )
				*cp = tolower( cc );
		}

		if (tok[0] == '#')
			continue;

		// see if have spilled over to the next material name:
		if (strcmp(tok, "newmtl") == 0)	// reached the end of this material
		{
			//glBegin(GL_TRIANGLES);
			return;
		}

		// process the rest of the line:

		float array[3];

		if( strcmp( tok, "ka" ) == 0 )
		{
			array[0] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			array[1] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			array[2] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,   array );
			//fprintf(stderr, "ka %6.2f %6.2f %6.2f\n", array[0], array[1], array[2]);
			continue;
		}

		if( strcmp( tok, "kd" ) == 0 )
		{
			array[0] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			array[1] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			array[2] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,   array );
			//fprintf(stderr, "kd %6.2f %6.2f %6.2f\n", array[0], array[1], array[2]);
			continue;
		}

		if( strcmp( tok, "ke" ) == 0 )
		{
			array[0] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			array[1] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			array[2] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION,   array );
			//fprintf(stderr, "ke %6.2f %6.2f %6.2f\n", array[0], array[1], array[2]);
			continue;
		}

		if( strcmp( tok, "ks" ) == 0 )
		{
			array[0] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			array[1] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			array[2] = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR,   array );
			//fprintf(stderr, "ks %6.2f %6.2f %6.2f\n", array[0], array[1], array[2]);
			continue;
		}

		if( strcmp( tok, "ns" ) == 0 )
		{
			float ns = (float)atof( strtok((char *)NULLPTR, DELIMS) );
			//if (ns > 20.f)
				//ns = 20.f;
			glMaterialf ( GL_FRONT_AND_BACK, GL_SHININESS, ns );
			//fprintf(stderr, "ns %6.2f\n", ns);
			continue;
		}

		if( strcmp( tok, "map_kd" ) == 0 )
		{
			tok = strtok( (char *)NULLPTR, DELIMS );
			char *suffix = GetSuffix( tok );
			if( strcmp( suffix, "bmp" ) != 0 )
			{
				fprintf( stderr, "Texture file '%s' needs to be a .bmp", tok );
			}
			else
			{
				int width, height;
				unsigned char * texture = BmpToTexture( tok, &width, &height );
				if( texture == NULL )
				{
					//fprintf( stderr, "Cannot open texture '%s'\n", tok );
				}
				else
				{
					//fprintf(stderr, "Opened texture '%s'\n", tok );
					//glEnd();
					glEndList();				// end the current display list

					GLuint texBind;
					glGenTextures( 1, &texBind );

					glBindTexture( GL_TEXTURE_2D, texBind );
					glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);
					delete [ ] texture;

					// create a new dl:

					GLuint dl = glGenLists(1);
					DisplayLists.push_back(dl);
					glNewList(dl, GL_COMPILE);


					glEnable( GL_TEXTURE_2D );
					glBindTexture(GL_TEXTURE_2D, texBind);
					glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

					//glBegin(GL_TRIANGLES);
				}
			}
			continue;
		}


		if (strcmp(tok, "tr") == 0)
			continue;
		if (strcmp(tok, "tf") == 0)
			continue;
		if (strcmp(tok, "d") == 0)
			continue;
		if (strcmp(tok, "ni") == 0)
			continue;
		if (strcmp(tok, "illum") == 0)
			continue;
		if (strcmp(tok, "map_ka") == 0)
			continue;
		if (strcmp(tok, "map_ks") == 0)
			continue;
		if (strcmp(tok, "map_bump") == 0)
			continue;
		if (strcmp(tok, "map_refl") == 0)
			continue;


#ifdef NOTYET
		if( strcmp( tok, "tr" ) == 0 )
		{
			tok = strtok( (char *)NULLPTR, DELIMS );
			thisMtl->Tr = atof( tok );
			thisMtl->HasTr = true;
			break;
		}

		if( strcmp( tok, "tf" ) == 0 )
		{
			tok = strtok( (char *)NULLPTR, DELIMS );
			thisMtl->Tf[0] = atof( tok );
			tok = strtok( (char *)NULLPTR, DELIMS );
			thisMtl->Tf[1] = atof( tok );
			tok = strtok( (char *)NULLPTR, DELIMS );
			thisMtl->Tf[2] = atof( tok );
			thisMtl->HasTf = true;
			break;
		}

		if( strcmp( tok, "d" ) == 0 )
		{
			tok = strtok( (char *)NULLPTR, DELIMS );
			thisMtl->D = atof( tok );
			thisMtl->HasD = true;
			break;
		}

		if( strcmp( tok, "ni" ) == 0 )
		{
			tok = strtok( (char *)NULLPTR, DELIMS );
			thisMtl->Ni = atof( tok );
			thisMtl->HasNi = true;
			break;
		}

		if( strcmp( tok, "illum" ) == 0 )
		{
			tok = strtok( (char *)NULLPTR, DELIMS );
			thisMtl->Illum = atof( tok );
			thisMtl->HasIllum = true;
			break;
		}

		if( strcmp( tok, "map_ka" ) == 0 )
		{
			tok = strtok( (char *)NULLPTR, DELIMS );
			thisMtl->MapKa = strdup( tok );
			thisMtl->HasMapKa = true;
			int width, height;
			unsigned char * texture = BmpToTexture( thisMtl->MapKa, &width, &height );
			if( texture == NULL )
			{
				fprintf( stderr, "Cannot open texture '%s'\n", thisMtl->MapKa );
				thisMtl->HasMapKa = false;
			}
			else
			{
				//fprintf(stderr, "Opened texture '%s'\n", thisMtl->MapKa);
				glGenTextures( 1, &thisMtl->BindKaTexture );
				fprintf( stderr, "BindKaTexture = %d\n", thisMtl->BindKaTexture );
				glBindTexture( GL_TEXTURE_2D, thisMtl->BindKaTexture );
				glEnable( GL_TEXTURE_2D );
				glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
				glTexImage2D( GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture );
				delete [ ] texture;
			}
			break;
		}

		if( strcmp( tok, "map_bump" ) == 0 )
		{
			thisMtl->bumpFactor = 1.0;
			tok = strtok( (char *)NULLPTR, DELIMS );
			thisMtl->MapBump = strdup(tok);
			if( tok[0] == '-' )
			{
				if( strcmp( tok, "-bm" ) == 0 )
				{
					tok = strtok( (char *)NULLPTR, DELIMS );
					thisMtl->bumpFactor = atof( tok );
					fprintf( stderr, "bumpFactor = %6.3f\n", thisMtl->bumpFactor );
					tok = strtok((char *)NULLPTR, DELIMS);
					thisMtl->MapBump = strdup(tok);
				}
				else
				{
					fprintf( stderr, "Unknown option in map_bump: '%s'\n", tok );
				}
			}
			thisMtl->HasMapBump = true;
			int width, height;
			unsigned char * texture = BmpToTexture( thisMtl->MapBump, &width, &height );
			if( texture == NULL )
			{
				fprintf( stderr, "Cannot open texture '%s'\n", thisMtl->MapBump );
				thisMtl->HasMapBump = false;
			}
			else
			{
				thisMtl->HasMapBump = false;
				fprintf( stderr, "Width = %d ; Height = %d\n", width, height );
				//glGenTextures( 1, &thisMtl->BindBumpTexture );
				//glBindTexture( GL_TEXTURE_2D, thisMtl->BindBumpTexture );
				//glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
				//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
				//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
				//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				//glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
				//glTexImage2D( GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture );
				delete [ ] texture;
			}
			break;
		}

		if( strcmp( tok, "map_refl" ) == 0 )
		{
			tok = strtok( (char *)NULLPTR, DELIMS );
			thisMtl->MapRefl = strdup( tok );
			thisMtl->HasMapRefl = true;
			int width, height;
			unsigned char * texture = BmpToTexture( thisMtl->MapRefl, &width, &height );
			if( texture == NULL )
			{
				fprintf( stderr, "Cannot open texture '%s'\n", thisMtl->MapRefl );
				thisMtl->HasMapRefl = false;
			}
			else
			{
				fprintf( stderr, "Width = %d ; Height = %d\n", width, height );
				glGenTextures( 1, &thisMtl->BindReflTexture );
				glBindTexture( GL_TEXTURE_2D, thisMtl->BindReflTexture );
				glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
				glTexImage2D( GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture );
				delete [ ] texture;
			}
			break;
		}
#endif

		fprintf( stderr, "Don't recognize Mtl variable '%s'\n", tok );
	}

	//glBegin(GL_TRIANGLES);
}
#endif

// return a pointer to the filename suffix
//	the '.' is not included


char *
GetSuffix( char *filename )
{
	// look backwards from the end of the filename, looking for the first '.'

	for( char * cp = &filename[ strlen(filename) - 1 ]; cp >= &filename[0]; cp-- )
	{
		char c = *cp;
		if( c == '.' )
		{
			return cp+1;
		}
	}

	// if ran off the beginning of the string, return empty string:

	return &filename[ strlen(filename) ];
}





//#define TEST

#ifdef TEST

int
main( int argc, char *argv[ ] )
{
	char *mtlFilename;

	if( argc == 1 )
	{
		//fprintf( stderr, "Usage: %s mtlFilename.mtl\n", argv[0] );
		//return 1;
		mtlFilename = (char *)"LunarModule.mtl";
	}
	else
	{
		mtlFilename = argv[1];
	}

	Mtls myMaterials;
	if(  myMaterials.Open( mtlFilename ) != 0 )
	{
		fprintf( stderr, "Could not read the mtl file\n" );
		return 1;
	}
	myMaterials.ReadMtlFile( );
	myMaterials.Close( );

	fprintf( stderr, "\n\nTesting 'blinn3SG'\n" );
	Mtl * blinnMtl = myMaterials.FindMtl( (char *)"blinn3SG" );
	if( blinnMtl != (Mtl *)NULLPTR )
	{
		fprintf( stderr, "Kd = %9.5f, %9.5f, %9.5f\n",
			blinnMtl->Kd[0], blinnMtl->Kd[1], blinnMtl->Kd[2] );
	}


	return 0;
}
#endif
