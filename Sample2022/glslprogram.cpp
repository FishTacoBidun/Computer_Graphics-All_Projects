#include "glslprogram.h"


struct GLshadertype
{
	char *extension;
	GLenum name;
}
ShaderTypes [ ] =
{
	{ (char *)".vert", VERTEX_SHADER_TYPE },
	{ (char *)".vs",   VERTEX_SHADER_TYPE },
	{ (char *)".frag", FRAGMENT_SHADER_TYPE },
	{ (char *)".fs",   FRAGMENT_SHADER_TYPE },

// note: glslprogram.h automatically undefines GEOMETRY, TESSELLATION, amd COMPUTE if this is on an APPLE:

#ifdef GEOMETRY
	{ (char *)".geom", GEOMETRY_SHADER_TYPE },
	{ (char *)".gs",   GEOMETRY_SHADER_TYPE },
#endif

#ifdef TESSELLATION
	{ (char *)".tcs", TESS_CONTROL_SHADER_TYPE },
	{ (char *)".tes", TESS_EVALUATION_SHADER_TYPE },
#endif

#ifdef COMPUTE
	{ (char *)".comp", COMPUTE_SHADER_TYPE },
	{ (char *)".cs",   COMPUTE_SHADER_TYPE },
#endif
};

static
char *
GetExtension( char *file )
{
	int n = (int)strlen(file) - 1;	// index of last non-null character

	// look for a '.':

	do
	{
		if( file[n] == '.' )
			return &file[n];	// the extension includes the '.'
		n--;
	} while( n >= 0 );

	// never found a '.':

	return NULL;
}


GLSLProgram::GLSLProgram( )
{
	// Init( );
	//Verbose = false;
	//CanDoComputeShaders = true;
	//CanDoVertexShaders = true;
	//CanDoTessellationShaders = true;
	//CanDoGeometryShaders = true;
	//CanDoFragmentShaders = true;
	//Valid = false;
}


// this is what is exposed to the user
// file1 - file5 are defaulted as NULL if not given
// CreateHelper is a varargs procedure, so must end in a NULL argument,
//	which I know to supply but I'm worried users won't

bool
GLSLProgram::Create( char *file0, char *file1, char *file2, char *file3, char * file4, char *file5 )
{
	return CreateHelper( file0, file1, file2, file3, file4, file5, NULL );
}


// this is the varargs version of the Create method

bool
GLSLProgram::CreateHelper( char *file0, ... )
{
	GLsizei n = 0;
	GLchar *buf;

	Valid = true;
	Vshader = Fshader = 0;
#ifdef GEOMETRY
	Gshader = 0;
#endif
#ifdef TESSELLATION
	TCshader = TEshader = 0;
#endif
#ifdef COMPUTE
	Cshader = 0;
#endif

	AttributeLocs.clear();
	UniformLocs.clear();

	Program = glCreateProgram( );
	CheckGlErrors( "glCreateProgram" );

	va_list args;
	va_start( args, file0 );

	// This is a little dicey
	// There is no way, using var args, to know how many arguments were passed
	// I am depending on the caller passing in a NULL as the final argument.
	// If they don't, bad things will happen.
	// But, this "should" work ok because the prototype for ::Create defaults all filenames after the first one to NULL

	char *file = file0;
	int type;
	while( file != NULL )
	{
		type = -1;
		char *extension = GetExtension( file );
		// fprintf( stderr, "File = '%s', extension = '%s'\n", file, extension );

		int maxShaderTypes = sizeof(ShaderTypes) / sizeof(struct GLshadertype);
		for( int i = 0; i < maxShaderTypes; i++ )
		{
			if( strcmp( extension, ShaderTypes[i].extension ) == 0 )
			{
				// fprintf( stderr, "Legal extension = '%s'\n", extension );
				type = i;
				break;
			}
		}

		GLuint shader;
		bool SkipToNextVararg = false;
		if( type < 0 )
		{
			fprintf( stderr, "Unknown filename extension: '%s'\n", extension );
			fprintf( stderr, "Legal Extensions are: " );
			for( int i = 0; i < maxShaderTypes; i++ )
			{
				if( i != 0 )	fprintf( stderr, " , " );
				fprintf( stderr, "%s", ShaderTypes[i].extension );
			}
			fprintf( stderr, "\n" );
			Valid = false;
			SkipToNextVararg = true;
		}

		if( ! SkipToNextVararg )
		{
			switch( ShaderTypes[type].name )
			{
				case VERTEX_SHADER_TYPE:
					if( ! CanDoVertexShaders )
					{
						fprintf( stderr, "Warning: this system cannot handle vertex shaders\n" );
						Valid = false;
						SkipToNextVararg = true;
					}
					else
					{
						shader = glCreateShader( GL_VERTEX_SHADER );
					}
					break;

				case FRAGMENT_SHADER_TYPE:
					if( ! CanDoFragmentShaders )
					{
						fprintf( stderr, "Warning: this system cannot handle fragment shaders\n" );
						Valid = false;
						SkipToNextVararg = true;
					}
					else
					{
						shader = glCreateShader( GL_FRAGMENT_SHADER );
					}
					break;

				case GEOMETRY_SHADER_TYPE:
					if( ! CanDoGeometryShaders )
					{
						fprintf( stderr, "Warning: this system cannot handle geometry shaders\n" );
						Valid = false;
						SkipToNextVararg = true;
					}
#ifdef GEOMETRY
					else
					{
						shader = glCreateShader( GL_GEOMETRY_SHADER );
					}
#endif
					break;

				case TESS_CONTROL_SHADER_TYPE:
					if( ! CanDoTessellationShaders )
					{
						fprintf( stderr, "Warning: this system cannot handle tessellation control shaders\n" );
						Valid = false;
						SkipToNextVararg = true;
					}
#ifdef TESSELLATION
					else
					{
						shader = glCreateShader( GL_TESS_CONTROL_SHADER );
					}
#endif
					break;

				case TESS_EVALUATION_SHADER_TYPE:
					if( ! CanDoTessellationShaders )
					{
						fprintf( stderr, "Warning: this system cannot handle tessellation evaluation shaders\n" );
						Valid = false;
						SkipToNextVararg = true;
					}
#ifdef TESSELLATION
					else
					{
						shader = glCreateShader( GL_TESS_EVALUATION_SHADER );
					}
#endif
					break;

				case COMPUTE_SHADER_TYPE:
					if( ! CanDoComputeShaders )
					{
						fprintf( stderr, "Warning: this system cannot handle compute shaders\n" );
						Valid = false;
						SkipToNextVararg = true;
					}
#ifdef COMPUTE
					else
					{
						shader = glCreateShader( GL_COMPUTE_SHADER );
					}
#endif
					break;
			}
		}


		// read the shader source into a buffer:

		if( ! SkipToNextVararg )
		{
			FILE * in;
			int length;
			FILE * logfile;
#ifdef WIN32
			errno_t err = fopen_s( &in, file, "rb" );
			if( err != 0 )
#else
			in = fopen( file, "rb" );
			if( in == NULL )
#endif
			{
				fprintf( stderr, "Cannot open shader file '%s'\n", file );
				Valid = false;
				SkipToNextVararg = true;
			}

			if( ! SkipToNextVararg )
			{
				fseek( in, 0, SEEK_END );
				length = ftell( in );
				fseek( in, 0, SEEK_SET );		// rewind

				buf = new GLchar[length+1];
				fread( buf, sizeof(GLchar), length, in );
				buf[length] = '\0';
				fclose( in ) ;

				GLchar *strings[2];
				int n = 0;
				strings[n] = buf;
				n++;

				// Tell GL about the source:

				glShaderSource( shader, n, (const GLchar **)strings, NULL );
				delete [ ] buf;
				CheckGlErrors( "Shader Source" );

				// compile:

				glCompileShader( shader );
				GLint infoLogLen;
				GLint compileStatus;
				CheckGlErrors( "CompileShader:" );
				glGetShaderiv( shader, GL_COMPILE_STATUS, &compileStatus );

				if( compileStatus == 0 )
				{
					fprintf( stderr, "Shader '%s' did not compile.\n", file );
					glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infoLogLen );
					if( infoLogLen > 0 )
					{
						GLchar *infoLog = new GLchar[infoLogLen+1];
						glGetShaderInfoLog( shader, infoLogLen, NULL, infoLog);
						infoLog[infoLogLen] = '\0';
#ifdef WIN32
						errno_t err = fopen_s( &logfile, "glsllog.txt", "w" );
						if( err != 0 )
#else
						logfile = fopen( "glsllog.txt", "w");
						if( logfile != NULL )
#endif
						{
							fprintf( logfile, "\n%s\n", infoLog );
							fclose( logfile );
						}
						fprintf( stderr, "\n%s\n", infoLog );
						delete [ ] infoLog;
					}
					glDeleteShader( shader );
					Valid = false;
				}
				else
				{
					if( Verbose )
						fprintf( stderr, "Shader '%s' compiled.\n", file );

					glAttachShader( this->Program, shader );
				}
			}
		}



		// go to the next vararg file:

		file = va_arg( args, char * );
	}

	va_end( args );

	// link the entire shader program:

	glLinkProgram( Program );
	CheckGlErrors( "Link Shader 1");

	GLchar* infoLog;
	GLint infoLogLen;
	GLint linkStatus;
	glGetProgramiv( this->Program, GL_LINK_STATUS, &linkStatus );
	CheckGlErrors("Link Shader 2");

	if( linkStatus == 0 )
	{
		glGetProgramiv( this->Program, GL_INFO_LOG_LENGTH, &infoLogLen );
		fprintf( stderr, "Failed to link program -- Info Log Length = %d\n", infoLogLen );
		if( infoLogLen > 0 )
		{
			infoLog = new GLchar[infoLogLen+1];
			glGetProgramInfoLog( this->Program, infoLogLen, NULL, infoLog );
			infoLog[infoLogLen] = '\0';
			fprintf( stderr, "Info Log:\n%s\n", infoLog );
			delete [ ] infoLog;

		}
		glDeleteProgram( Program );
		Valid = false;
	}
	else
	{
		if( Verbose )
			fprintf( stderr, "Shader Program linked.\n" );
		// validate the program:

		GLint status;
		glValidateProgram( Program );
		glGetProgramiv( Program, GL_VALIDATE_STATUS, &status );
		if( status == GL_FALSE )
		{
			fprintf( stderr, "Program is invalid.\n" );
			Valid = false;
		}
		else
		{
			if( Verbose )
				fprintf( stderr, "Shader Program validated.\n" );
		}
	}

	return Valid;
}


void
GLSLProgram::DisableVertexAttribArray( const char *name )
{
	int loc;
	if( ( loc = GetAttributeLocation( (char *)name ) )  >= 0 )
	{
		this->Use();
		glDisableVertexAttribArray( loc );
	}
}



void
GLSLProgram::EnableVertexAttribArray( const char *name )
{
	int loc;
	if( ( loc = GetAttributeLocation( (char *)name ) )  >= 0 )
	{
		this->Use();
		glEnableVertexAttribArray( loc );
	}
}


int
GLSLProgram::GetUniformTypeAndSize( GLchar *name, GLint *sizep, GLenum *typep )
{
	int numactiveuniforms;
	glGetProgramiv( Program, GL_ACTIVE_UNIFORMS, &numactiveuniforms );
	if( Verbose )
		fprintf( stderr, "numactiveuniforms = %d\n", numactiveuniforms );

	int bufsize;
	glGetProgramiv( Program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufsize );
	char *lname = new char [bufsize+1];

	for( int i = 0; i < numactiveuniforms; i++ )
	{
		GLint lsize;
		GLenum ltype;
		glGetActiveUniform( Program, i, bufsize, NULL, &lsize, &ltype, lname );
		if( strcmp( name, lname ) == 0 )
		{
			if( Verbose )
				fprintf( stderr, "Uniform Variable #%2d, '%s' is size %d and type 0x%X\n", i, lname, lsize, ltype );
			*sizep = lsize;
			*typep = ltype;
			return 0;
		}
	}

	return -1;
}


int
GLSLProgram::GetAttributeTypeAndSize( GLchar *name, GLint *sizep, GLenum *typep )
{
	int numactiveattribs;
	glGetProgramiv( Program, GL_ACTIVE_ATTRIBUTES, &numactiveattribs );
	if( Verbose )
		fprintf( stderr, "numactiveattribs = %d\n", numactiveattribs );

	//glGetProgramiv( Program, GL_ACTIVE_ATTRIB_MAX_LENGTH, &bufsize );
	GLsizei bufsize = 256;
	char *lname = new char [bufsize+1];

	for( int i = 0; i < numactiveattribs; i++ )
	{
		GLint lsize;
		GLenum ltype;
		GLsizei length;
		glGetActiveAttrib( Program, i, bufsize, &length, &lsize, &ltype, lname );
		if( strcmp( name, lname ) == 0 )
		{
			if( Verbose )
				fprintf( stderr, "Attribute Variable #%2d, '%s' is size %d and type 0x%X\n", i, lname, lsize, ltype );
			*sizep = lsize;
			*typep = ltype;
			return 0;
		}
	}

	return -1;
}


void
GLSLProgram::Init( )
{
	Verbose = false;
	const GLubyte* extensions = NULL;
#ifndef __APPLE__
	extensions = glGetString(GL_EXTENSIONS);
#endif
	if( extensions != NULL )
	{
		//*********************************************************************************
		// use this to print all extensions:
		// (comment these out if you don't want to see the list)
		//int len = strlen((char*)extensions);
		//char* cp = (char*)extensions;
		//char c;
		//fprintf( stderr, "Here are all the extensions your system has:\n" );
		//while ((c = *cp++) != '\0')
		//{
			//if (c == ' ')	c = '\n';
			//fprintf(stderr, "%c", c);
		//}
		//fprintf( stderr, "\n" );
		//*********************************************************************************

		CanDoComputeShaders      = IsExtensionSupported( "GL_ARB_compute_shader" );
		CanDoVertexShaders       = IsExtensionSupported( "GL_ARB_vertex_shader" )     ||  IsExtensionSupported( "GL_EXT_vertex_shader" );
		CanDoTessellationShaders = IsExtensionSupported( "GL_ARB_tessellation_shader" );
		CanDoGeometryShaders     = IsExtensionSupported( "GL_ARB_geometry_shader4" )  ||  IsExtensionSupported( "GL_EXT_geometry_shader4" ) || IsExtensionSupported("GL_EXT_geometry_shader");
		CanDoFragmentShaders     = IsExtensionSupported( "GL_ARB_fragment_shader" );
		fprintf( stderr, "This system can handle:\n" );
	}
	else
	{
		CanDoComputeShaders      = true;
		CanDoVertexShaders       = true;
		CanDoTessellationShaders = true;
		CanDoGeometryShaders     = true;
		CanDoFragmentShaders     = true;
		fprintf( stderr, "Your system's OpenGL is not telling me what extensions you have.\n" );
		fprintf( stderr, "So, I am going to assume that your system can handle:\n" );
	}

	if( CanDoVertexShaders )                fprintf( stderr, "\tvertex shaders \n" );
	if( CanDoFragmentShaders )              fprintf( stderr, "\tfragment shaders \n" );
	if( CanDoGeometryShaders )              fprintf( stderr, "\tgeometry shaders \n" );
	if( CanDoTessellationShaders )          fprintf( stderr, "\ttessellation control shaders \n" );
	if( CanDoTessellationShaders )          fprintf( stderr, "\ttessellation evaluation shaders \n" );
	if( CanDoComputeShaders )               fprintf( stderr, "\tcompute shaders \n");

	fprintf( stderr, "\n" );
}


bool
GLSLProgram::IsValid( )
{
	return Valid;
}


bool
GLSLProgram::IsNotValid( )
{
	return ! Valid;
}


void
GLSLProgram::SetVerbose( bool v )
{
	Verbose = v;
}

void
GLSLProgram::UnUse( )
{
	Use( 0 );
}


void
GLSLProgram::Use( )
{
	Use( this->Program );
};


void
GLSLProgram::Use( GLuint p )
{
	if( p != CurrentProgram )
	{
		glUseProgram( p );
		CurrentProgram = p;
	}
};


void
GLSLProgram::UseFixedFunction( )
{
	this->Use( 0 );
};


#ifdef COMPUTE
void
GLSLProgram::DispatchCompute( int num_groups_x, int num_groups_y, int num_groups_z )
{
	this->Use( );
	glDispatchCompute( num_groups_x, num_groups_y, num_groups_z );
}
#endif


int
GLSLProgram::GetAttributeLocation( char *name )
{
	std::map<char *, int>::iterator pos;

	pos = AttributeLocs.find( name );
	if( pos == AttributeLocs.end() )
	{
		AttributeLocs[name] = glGetAttribLocation( this->Program, name );
	}

	return AttributeLocs[name];
};


void
GLSLProgram::SetAttributePointer3fv( char* name, float* vals )
{
	int loc;
	if( ( loc = GetAttributeLocation( name ) )  >= 0 )
	{
		this->Use();
		glVertexAttribPointer( loc, 3, GL_FLOAT, GL_FALSE, 0, vals );
	}
};


#ifdef NOT_SUPPORTED_BY_OPENGL_FOR_SOME_STRANGE_REASON
void
GLSLProgram::SetAttributeVariable( char* name, int val )
{
	int loc;
	if( ( loc = GetAttributeLocation( name ) )  >= 0 )
	{
		this->Use();
		glVertexAttrib1i( loc, val );
	}
};
#endif


void
GLSLProgram::SetAttributeVariable( char* name, int val )
{
	int loc;
	if( ( loc = GetAttributeLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetAttributeTypeAndSize( name, &size, &type );

		switch( type )
		{
#ifdef NOT_SUPPORTED_BY_OPENGL_FOR_SOME_STRANGE_REASON
			case GL_INT:
				glVertexAttrib1i( loc, val );
				break;
#endif

			case GL_FLOAT:
				glVertexAttrib1f( loc, (float)val );
				break;

#ifndef __APPLE__
			case GL_DOUBLE:
				glVertexAttrib1d( loc, (double)val );
				break;
#endif

			default:
				fprintf( stderr, "Setting attribute variable '%s': please be more explicit with the variable type\n", name );
		}
#else
#ifdef NOT_SUPPORTED_BY_OPENGL_FOR_SOME_STRANGE_REASON
		glVertexAttrib1i( loc, val );
#endif
#endif
	}
};


void
GLSLProgram::SetAttributeVariable( char* name, float val )
{
	int loc;
	if( ( loc = GetAttributeLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetAttributeTypeAndSize( name, &size, &type );

		switch( type )
		{
#ifdef NOT_SUPPORTED_BY_OPENGL_FOR_SOME_STRANGE_REASON
			case GL_INT:
				glVertexAttrib1i( loc, (int)val );
				break;
#endif

			case GL_FLOAT:
				glVertexAttrib1f( loc, val );
				break;

			case GL_DOUBLE:
				glVertexAttrib1d( loc, (double)val );
				break;

			default:
				fprintf( stderr, "Setting attribute variable '%s': please be more explicit with the variable type\n", name );
		}
#else
		glVertexAttrib1f( loc, val );
#endif
	}
};


void
GLSLProgram::SetAttributeVariable( char* name, double val )
{
	int loc;
	if( ( loc = GetAttributeLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetAttributeTypeAndSize( name, &size, &type );

		switch( type )
		{
#ifdef NOT_SUPPORTED_BY_OPENGL_FOR_SOME_STRANGE_REASON
			case GL_INT:
				glVertexAttrib1i( loc, (int)val );
				break;
#endif

			case GL_FLOAT:
				glVertexAttrib1f( loc, (float)val );
				break;

#ifndef __APPLE__
			case GL_DOUBLE:
				glVertexAttrib1d( loc, val );
				break;
#endif

			default:
				fprintf( stderr, "Setting attribute variable '%s': please be more explicit with the variable type\n", name );
		}
#else
#ifndef __APPLE__
		glVertexAttrib1d( loc, val );
#endif
#endif
	}
};


void
GLSLProgram::SetAttributeVariable( char* name, float val0, float val1, float val2 )
{
	int loc;
	if( ( loc = GetAttributeLocation( name ) )  >= 0 )
	{
		this->Use();
		glVertexAttrib3f( loc, val0, val1, val2 );
	}
};


void
GLSLProgram::SetAttributeVariable( char* name, float vals[3] )
{
	int loc;
	if( ( loc = GetAttributeLocation( name ) )  >= 0 )
	{
		this->Use();
		glVertexAttrib3fv( loc, vals );
	}
};


int
GLSLProgram::GetUniformLocation( char *name )
{
	std::map<char *, int>::iterator pos;

	pos = UniformLocs.find( name );
	//if( Verbose )
		//fprintf( stderr, "Uniform: pos = 0x%016x ; size = %d ; end = 0x%016x\n", pos, UniformLocs.size(), UniformLocs.end() );
	if( pos == UniformLocs.end() )
	{
		GLuint loc = glGetUniformLocation( this->Program, name );
		UniformLocs[name] = loc;
		if( Verbose )
			fprintf( stderr, "Location of '%s' in Program %d = %d\n", name, this->Program, loc );
	}
	else
	{
		if( Verbose )
		{
			fprintf( stderr, "Location = %d\n", UniformLocs[name] );
			if( UniformLocs[name] == -1 )
				fprintf( stderr, "Location of uniform variable '%s' is -1\n", name );
		}
	}

	return UniformLocs[name];
};


void
GLSLProgram::SetUniformVariable( char* name, bool val )
{
	int loc;
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );

		switch( type )
		{
			case GL_BOOL:
				glUniform1i( loc, val ?  1 : 0 );
				break;

			default:
				fprintf( stderr, "Uniform variable '%s': is not a bool like you are treating it as\n", name );
				glUniform1i( loc, val );
		}
	}

};


void
GLSLProgram::SetUniformVariable( char* name, int val )
{
	int loc;
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );

		switch( type )
		{
			case GL_INT:
			case GL_SAMPLER_1D:
			case GL_SAMPLER_2D:
			case GL_SAMPLER_3D:
			case GL_SAMPLER_CUBE:
			case GL_SAMPLER_1D_SHADOW:
			case GL_SAMPLER_2D_SHADOW:
				glUniform1i( loc, val );
				break;

			case GL_FLOAT:
				glUniform1f( loc, (float)val );
				break;

#ifndef __APPLE__
			case GL_DOUBLE:
				glUniform1d( loc, (double)val );
				break;
#endif

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
				glUniform1i( loc, val );
		}
#else
		glUniform1i( loc, val );
#endif
	}

};


void
GLSLProgram::SetUniformVariable( char* name, float val )
{
	int loc;
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );

		switch( type )
		{
			case GL_INT:
				glUniform1i( loc, (int)val );
				break;

			case GL_FLOAT:
				glUniform1f( loc, val );
				break;

#ifndef __APPLE__
			case GL_DOUBLE:
				glUniform1d( loc, (double)val );
				break;
#endif

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
		}
#else
		glUniform1f( loc, val );
#endif
	}
};


void
GLSLProgram::SetUniformVariable( char* name, double val )
{
	int loc;
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );
		switch( type )
		{
			case GL_INT:
				glUniform1i( loc, (int)val );
				break;

			case GL_FLOAT:
				glUniform1f( loc, (float)val );
				break;

#ifndef __APPLE__
			case GL_DOUBLE:
				glUniform1d( loc, val );
				break;
#endif

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
		}
#else
#ifndef __APPLE__
		glUniform1d( loc, val );
#endif
#endif
	}
};


void
GLSLProgram::SetUniformVariable(char* name, float val0, float val1 )
{
	int loc;
	if ((loc = GetUniformLocation(name)) >= 0)
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize(name, &size, &type);
		switch (type)
		{
			case GL_FLOAT_VEC2:
				glUniform2f(loc, val0, val1);
				break;

			case GL_FLOAT_VEC3:
				glUniform3f(loc, val0, val1, 0.f);
				break;

			case GL_FLOAT_VEC4:
				glUniform4f(loc, val0, val1, 0.f, 1.f);
				break;

			default:
				fprintf(stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name);
				glUniform2f(loc, val0, val1 );
		}
#else
		glUniform2f(loc, val0, val1 );
#endif
	}
};

void
GLSLProgram::SetUniformVariable( char* name, float val0, float val1, float val2 )
{
	int loc;
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );
		switch( type )
		{
			case GL_FLOAT_VEC2:
				glUniform2f( loc, val0, val1 );
				break;

			case GL_FLOAT_VEC3:
				glUniform3f( loc, val0, val1, val2 );
				break;

			case GL_FLOAT_VEC4:
				glUniform4f( loc, val0, val1, val2, 1.f );
				break;

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
		}
#else
		glUniform3f( loc, val0, val1, val2 );
#endif
	}
};


void
GLSLProgram::SetUniformVariable( char* name, float val0, float val1, float val2, float val3 )
{
	int loc;
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );
		switch( type )
		{
			case GL_FLOAT_VEC2:
				glUniform2f( loc, val0, val1 );
				break;

			case GL_FLOAT_VEC3:
				glUniform3f( loc, val0, val1, val2 );
				break;

			case GL_FLOAT_VEC4:
				glUniform4f( loc, val0, val1, val2, val3 );
				break;

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
				glUniform4f( loc, val0, val1, val2, val3 );
		}
#else
		glUniform4f( loc, val0, val1, val2, val3 );
#endif
	}
};


void
GLSLProgram::SetUniformVariable( char* name, float vals[3] )
{
	int loc;

	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );
		switch( type )
		{
			case GL_FLOAT_VEC2:
				glUniform2fv( loc, 1, vals );
				break;

			case GL_FLOAT_VEC3:
				glUniform3fv( loc, 1, vals );
				break;

			case GL_FLOAT_VEC4:
				glUniform4f( loc, vals[0], vals[1], vals[2], 1. );
				break;

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
				glUniform3fv( loc, 1, vals );
		}
#else
		glUniform3fv( loc, 1, vals );
#endif
	}
};


//********************************************************************************
#ifdef GLM

void
GLSLProgram::SetUniformVariable( char *name, glm::vec2 v2 )
{
	int loc;
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );
		switch( type )
		{
			case GL_FLOAT_VEC2:
				glUniform2f( loc, v2.x, v2.y );
				break;

			case GL_FLOAT_VEC3:
				glUniform3f( loc, v2.x, v2.y, 0. );
				break;

			case GL_FLOAT_VEC4:
				glUniform4f( loc, v2.x, v2.y, 0., 1. );
				break;

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
				glUniform2f( loc, v2.x, v2.y );
		}
#else
		glUniform2f( loc, v2.x, v2.y );
#endif
	}
};


void
GLSLProgram::SetUniformVariable( char *name, glm::vec3 v3 )
{
	int loc;
	//fprintf( stderr, "Found a vec3\n" );
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );
		switch( type )
		{
			case GL_FLOAT_VEC2:
				glUniform2f( loc, v3.x, v3.y );
				break;

			case GL_FLOAT_VEC3:
				glUniform3f( loc, v3.x, v3.y, v3.z );
				break;

			case GL_FLOAT_VEC4:
				glUniform4f( loc, v3.x, v3.y, v3.z, 1. );
				break;

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
				glUniform3f( loc, v3.x, v3.y, v3.z );
		}
#else
		glUniform3f( loc, v3.x, v3.y, v3.z );
#endif
	}
};

void
GLSLProgram::SetUniformVariable( char *name, glm::vec4 v4 )
{
	int loc;
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );
		switch( type )
		{
			case GL_FLOAT_VEC2:
				glUniform2f( loc, v4.x, v4.y );
				break;

			case GL_FLOAT_VEC3:
				glUniform3f( loc, v4.x, v4.y, v4.z );
				break;

			case GL_FLOAT_VEC4:
				glUniform4f( loc, v4.x, v4.y, v4.z, v4.w );
				break;

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
				glUniform4f( loc, v4.x, v4.y, v4.z, v4.w );
		}
#else
		glUniform4f( loc, v4.x, v4.y, v4.z, v4.w );
#endif
	}
};


void
GLSLProgram::SetUniformVariable( char *name, glm::mat2 m2 )
{
	int loc;
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );
		switch( type )
		{
			case GL_FLOAT_MAT2:
				glUniformMatrix2fv( loc, 1, GL_FALSE, glm::value_ptr( m2 ) );
				break;

			case GL_FLOAT_MAT3:
				glm::mat3 m3 = glm::mat3( m2 );
				glUniformMatrix3fv( loc, 1, GL_FALSE, glm::value_ptr( m3 ) );
				break;

			case GL_FLOAT_MAT4:
				glm::mat4 m4 = glm::mat4( m2 );
				glUniformMatrix4fv( loc, 1, GL_FALSE, glm::value_ptr( m4 ) );
				break;

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
				glUniformMatrix2fv( loc, 1, GL_FALSE, glm::value_ptr( m2 ) );
		}
#else
		glUniformMatrix2fv( loc, 1, GL_FALSE, glm::value_ptr( m2 ) );
#endif
	}
};



void
GLSLProgram::SetUniformVariable( char *name, glm::mat3 m3 )
{
	int loc;
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );
		switch( type )
		{
			case GL_FLOAT_MAT2:
				glm::mat2 m2 = glm::mat2( m3 );
				glUniformMatrix2fv( loc, 1, GL_FALSE, glm::value_ptr( m2 ) );
				break;

			case GL_FLOAT_MAT3:
				glUniformMatrix3fv( loc, 1, GL_FALSE, glm::value_ptr( m3 ) );
				break;

			case GL_FLOAT_MAT4:
				glm::mat4 m4 = glm::mat4( m3 );
				glUniformMatrix4fv( loc, 1, GL_FALSE, glm::value_ptr( m4 ) );
				break;

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
				glUniformMatrix3fv( loc, 1, GL_FALSE, glm::value_ptr( m3 ) );
		}
#else
		glUniformMatrix3fv( loc, 1, GL_FALSE, glm::value_ptr( m3 ) );
#endif
	}
};


void
GLSLProgram::SetUniformVariable( char *name, glm::mat4 m4 )
{
	int loc;
	if( ( loc = GetUniformLocation( name ) )  >= 0 )
	{
		this->Use();
#ifdef TYPE_CHECKS
		GLint  size;
		GLenum type;
		GetUniformTypeAndSize( name, &size, &type );
		switch( type )
		{
			case GL_FLOAT_MAT2:
				glm::mat2 m2 = glm::mat2( m4 );
				glUniformMatrix2fv( loc, 1, GL_FALSE, glm::value_ptr( m2 ) );
				break;

			case GL_FLOAT_MAT3:
				glm::mat3 m3 = glm::mat3( m4 );
				glUniformMatrix3fv( loc, 1, GL_FALSE, glm::value_ptr( m3 ) );
				break;

			case GL_FLOAT_MAT4:
				glUniformMatrix4fv( loc, 1, GL_FALSE, glm::value_ptr( m4 ) );
				break;

			default:
				fprintf( stderr, "Setting uniform variable '%s': please be more explicit with the variable type\n", name );
				glUniformMatrix4fv( loc, 1, GL_FALSE, glm::value_ptr( m4 ) );
		}
#else
		glUniformMatrix4fv( loc, 1, GL_FALSE, glm::value_ptr( m4 ) );
#endif
	}
};

#endif
//********************************************************************************



bool
GLSLProgram::IsExtensionSupported( const char *extension )
{
	// see if the extension is bogus:

	if( extension == NULL  ||  extension[0] == '\0' )
		return false;

	GLubyte *where = (GLubyte *) strchr( extension, ' ' );
	if( where != 0 )
		return false;

	// get the full list of extensions:

	const GLubyte *extensions = glGetString( GL_EXTENSIONS );


	for( const GLubyte *start = extensions; ; )
	{
		where = (GLubyte *) strstr( (const char *) start, extension );
		if( where == 0 )
			return false;

		GLubyte *terminator = where + strlen(extension);

		if( where == start  ||  *(where - 1) == '\n'  ||  *(where - 1) == ' ' )
			if( *terminator == ' '  ||  *terminator == '\n'  ||  *terminator == '\0' )
				return true;
		start = terminator;
	}
	return false;
}


int GLSLProgram::CurrentProgram = 0;




#ifndef CHECK_GL_ERRORS
#define CHECK_GL_ERRORS
void
CheckGlErrors( const char* caller )
{
	unsigned int gle = glGetError();

	if( gle != GL_NO_ERROR )
	{
		fprintf( stderr, "GL Error discovered from caller %s: ", caller );
		switch (gle)
		{
			case GL_INVALID_ENUM:
				fprintf( stderr, "Invalid enum.\n" );
				break;
			case GL_INVALID_VALUE:
				fprintf( stderr, "Invalid value.\n" );
				break;
			case GL_INVALID_OPERATION:
				fprintf( stderr, "Invalid Operation.\n" );
				break;
			case GL_STACK_OVERFLOW:
				fprintf( stderr, "Stack overflow.\n" );
				break;
			case GL_STACK_UNDERFLOW:
				fprintf(stderr, "Stack underflow.\n" );
				break;
			case GL_OUT_OF_MEMORY:
				fprintf( stderr, "Out of memory.\n" );
				break;
		}
		return;
	}
}
#endif


//#define TEST
#ifdef TEST
#include "glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"


GLSLProgram Pattern;

int
main( )
{
        GLenum err = glewInit( );
        if( err != GLEW_OK )
        {
                fprintf( stderr, "glewInit Error\n" );
        }
        else
                fprintf( stderr, "GLEW initialized OK\n" );
        fprintf( stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	fprintf( stderr, "Starting\n" );
	Pattern.Init( );
	fprintf( stderr, "Called Init\n" );
	return 0;
}
#endif
