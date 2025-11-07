#ifndef GLSLPROGRAM_CPP
#define GLSLPROGRAM_CPP

#include <ctype.h>
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <math.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#endif


#ifdef __APPLE__
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#else
	#include "glew.h"
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif


void	CheckGlErrors( const char* );



//********************************************************************************
// #define this to allow this to accept geometry shaders:
	//#define GEOMETRY
//********************************************************************************


//********************************************************************************
// #define this to allow this to accept tessellation shaders:
	//#define TESSELLATION
//********************************************************************************


//********************************************************************************
// #define this to allow this to accept compute shaders:
	//#define COMPUTE
//********************************************************************************



//********************************************************************************
// #define this to force extra checks on the type of value being sent to an attribute
// or uniform variable to be sure that the value being written matches the variable
// type being written into
	#define TYPE_CHECKS
// the downside of enabling this is that it consumes extra time doing the checks,
// but for most use cases, it is definitely worth it
//********************************************************************************


#ifdef __APPLE__
	#undef GEOMETRY
	#undef TESSELLATION
	#undef COMPUTE
#endif


inline int GetOSU(int flag)
{
	int i;
	glGetIntegerv(flag, &i);
	return i;
}

#include "glut.h"
#include <map>
#include <stdarg.h>


//********************************************************************************
// #define GLM to allow this to accept glm uniform variables:
	//#define GLM

// i'm assuming that this code is #include'ed into the application code and that the
//	glm #includes are in that application program before this code is #included.
// if they're not, then these lines will be needed here:
#define GLM_FORCE_RADIANS
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat3x3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
//
//********************************************************************************

// shader types:
enum ShaderTypes
{
	VERTEX_SHADER_TYPE, FRAGMENT_SHADER_TYPE, GEOMETRY_SHADER_TYPE, TESS_CONTROL_SHADER_TYPE, TESS_EVALUATION_SHADER_TYPE, COMPUTE_SHADER_TYPE
};




class GLSLProgram
{
  private:
	std::map<char *, int>	AttributeLocs;
#ifdef COMPUTE
	char *			Cfile;
	unsigned int		Cshader;
#endif
	char *			Ffile;
	unsigned int		Fshader;
#ifdef GEOMETRY
	char *			Gfile;
	unsigned int		Gshader;
#endif
	bool			IncludeGstap;
	GLuint			Program;
#ifdef TESSELLATION
	char *			TCfile;
	unsigned int		TCshader;
	char *			TEfile;
	unsigned int		TEshader;
#endif
	std::map<char *, int>	UniformLocs;
	bool			Valid;
	char *			Vfile;
	GLuint			Vshader;
	bool			Verbose;

	static int		CurrentProgram;

	void	AttachShader( GLuint );
	bool	CanDoComputeShaders;
	bool	CanDoFragmentShaders;
	bool	CanDoGeometryShaders;
	bool	CanDoTessellationShaders;
	bool	CanDoVertexShaders;
	int	CompileShader( GLuint );
	bool	CreateHelper( char *, ... );
	int	GetAttributeLocation( char * );
	int	GetUniformLocation( char * );


  public:
		GLSLProgram( );

	bool	Create( char *, char * = NULL, char * = NULL, char * = NULL, char * = NULL, char * = NULL );
	void	DisableVertexAttribArray( const char * );
#ifdef COMPUTE
	void	DispatchCompute( int, int, int );
#endif
	void	EnableVertexAttribArray( const char * );
	int	GetAttributeTypeAndSize( GLchar *, GLint *, GLenum * );
	int	GetUniformTypeAndSize(   GLchar *, GLint *, GLenum * );
	void	Init( );
	bool	IsExtensionSupported( const char * );
	bool	IsNotValid( );
	bool	IsValid( );
	void	SetAttributePointer3fv( char *, float * );
	void	SetAttributeVariable( char *, int );
	void	SetAttributeVariable( char *, float );
	void	SetAttributeVariable( char *, double );
	void	SetAttributeVariable( char *, float, float, float );
	void	SetAttributeVariable( char *, float[3] );
	void	VertexAttrib3f( const char *, float, float, float );
	void	SetUniformVariable( char *, bool );
	void	SetUniformVariable( char *, int );
	void	SetUniformVariable( char *, float );
	void	SetUniformVariable( char *, double );
	void	SetUniformVariable( char*, float, float);
	void	SetUniformVariable( char *, float, float, float );
	void	SetUniformVariable( char *, float, float, float, float );
	void	SetUniformVariable( char *, float[3] );

#ifdef GLM
	void	SetUniformVariable( char *, glm::vec2 );
	void	SetUniformVariable( char *, glm::vec3 );
	void	SetUniformVariable( char *, glm::vec4 );
	void	SetUniformVariable( char *, glm::mat2 );
	void	SetUniformVariable( char *, glm::mat3 );
	void	SetUniformVariable( char *, glm::mat4 );
#endif

	void	SetVerbose( bool );
	void	UnUse( );
	void	Use( );
	void	Use( GLuint );
	void	UseFixedFunction( );
};

#endif		// #ifndef GLSLPROGRAM_CPP
