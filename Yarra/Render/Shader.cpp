/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2016, ZenYes Pty Ltd
	DESCRIPTION:	GLSL support (vertex and fragment shaders)
					This release performs minimal shader management
*/

#include <cassert>

#include "Yarra/Platform.h"
#include "Yarra/FileManager.h"

#include "SceneNode.h"
#include "Shader.h"
#include "MatrixStack.h"

namespace yrender
{

#define Y_DEBUG_MODE	1
	
/*************************************
	GLSL #version preprocessor directive.
	According to the spec, #version must occur at the beginning of the source code.
	This definition will be added to the shader source code.
**************************************/
#if defined(GL_ES_VERSION_2_0)
	static const char *kGLSLVersion = "#version 100\nprecision mediump float;\n";
#elif defined(__HAIKU__)
	static const char *kGLSLVersion = "#version 150\n";	//	Mesa 17.x
#else
	static const char *kGLSLVersion = "#version 150\n";	//	Similar to 330, but without location attribute.  Works with OSX Lion
#endif

#define SHADER_VERBOSITY_LEVEL		0		/*	0 - only errors, 1 - info log	*/

/*	FUNCTION:		YBuffer_ReadFileToMemory
	ARGUMENTS:		filename
					filesize
	RETURN:			pointer to memory (client takes ownership), NULL if error
	DESCRIPTION:	Copy filename into allocated memory
*/
char * YBuffer_ReadFileToMemory(const char *filename, size_t *filesize)
{
	//	Open file
	YFILE		*file;
	bool file_exists;
	do
	{
		file_exists = true;
		file = Yfopen(filename, "rb");
		if (!file)
		{
#if Y_DEBUG_MODE
			yplatform::Debug("YBuffer_ReadFileToMemory(%s) - file not found\n", filename);
			file_exists = false;
			assert(0);
#else
			yplatform::Exit("YBuffer_ReadFileToMemory(%s) - file not found\n", filename);
#endif
		}
	} while (!file_exists);
	
	//	Determine file size
	Yfseek (file, 0, SEEK_END);
	*filesize = Yftell(file);
	Yfseek(file, 0, SEEK_SET);

	if (*filesize == 0)
	{
		yplatform::Debug("YBuffer_ReadFileToMemory(%s) - zero length file\n", filename);
		Yfclose(file);
		assert(0);
	}

	//	Read file to memory
	char *data = new char[*filesize + 1];	// reserve space for zero terminator
	if (data)
	{
		Yfread(data, 1, *filesize, file);
		data[*filesize] = 0;			//	terminator
	}
	else
	{
		yplatform::Debug("YBuffer_ReadFileToMemory(%s) - insufficient memory\n", filename);
		Yfclose(file);
		assert(0);
	}

	Yfclose(file);
	return data;
}


/*************************************
	YShader
**************************************/

/*	FUNCTION:		YShader :: YShader
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Constructor.
*/
YShader :: YShader()
{
	fProgram = 0;
	fNumberAttributes = 0;
}

/*	FUNCTION:		YShader :: YShader
	ARGUMENTS:		vertex_shader_source_file
					fragment_shader_source_file
					attribute_binding
					fragdata_binding
					vertex_shader_patch				// prepend vertex shader with source code
					fragment_shader_patch			// prepend fragment shader with source code
	RETURN:			n/a
	DESCRIPTION:	Constructor.  Allow dynamic modification of source code at run time.
*/
YShader :: YShader(	const char *vertex_shader_source_file,
					const char *fragment_shader_source_file,
					const std::vector<std::string> *attribute_binding,
					const std::vector<std::string> *fragdata_binding,
					const char *vertex_shader_patch,
					const char *fragment_shader_patch)
{
	fProgram = 0;
	fNumberAttributes = 0;

	GLuint vertex_object = 0, fragment_object = 0;
	bool success;
	do
	{
		success = true;
		try
		{
			if (vertex_shader_source_file)
				vertex_object = CreateObjectFromFile(vertex_shader_source_file, GL_VERTEX_SHADER, vertex_shader_patch);
			if (fragment_shader_source_file)
				fragment_object = CreateObjectFromFile(fragment_shader_source_file, GL_FRAGMENT_SHADER, fragment_shader_patch);

			if ((vertex_object != 0) && (fragment_object != 0))
				CreateProgram(vertex_object, fragment_object, attribute_binding, fragdata_binding);
		}
		catch (...)
		{
			FreeObject(vertex_object);
			FreeObject(fragment_object);
			FreeProgram();
			success = false;
#if Y_DEBUG_MODE
			yplatform::Debug("YShader() error (%s, %s)\n", vertex_shader_source_file, fragment_shader_source_file);
			return;	//	assert(0);
#else
			yplatform::Exit("YShader() error (%s, %s)\n", vertex_shader_source_file, fragment_shader_source_file);
			return;
#endif
		}
	} while (!success);

	FreeObject(vertex_object);
	FreeObject(fragment_object);
}

/*	FUNCTION:		YShader :: YShader
	ARGUMENTS:		attribute_binding
					vertex_shader_source_text
					fragment_shader_source_text
					fragdata_binding
					vertex_shader_patch				// prepend vertex shader with source code
					fragment_shader_patch			// prepend fragment shader with source code
	RETURN:			n/a
	DESCRIPTION:	Constructor.  Allow dynamic modification of source code at run time.
*/
YShader :: YShader(	const std::vector<std::string> *attribute_binding,
					const char *vertex_shader_source_text,
					const char *fragment_shader_source_text,
					const std::vector<std::string> *fragdata_binding)
{
	fProgram = 0;
	fNumberAttributes = 0;

	GLuint vertex_object = 0, fragment_object = 0;
	bool success;
	do 
	{
		success = true;
		try
		{
			if (vertex_shader_source_text)
				vertex_object = CreateObjectFromMemory(vertex_shader_source_text, GL_VERTEX_SHADER);
			if (fragment_shader_source_text)
				fragment_object = CreateObjectFromMemory(fragment_shader_source_text, GL_FRAGMENT_SHADER);
	
			if ((vertex_object != 0) && (fragment_object != 0))
				CreateProgram(vertex_object, fragment_object, attribute_binding, fragdata_binding);
		}
		catch (...)
		{
			FreeObject(vertex_object);
			FreeObject(fragment_object);
			FreeProgram();
			success = false;
#if Y_DEBUG_MODE
			yplatform::Debug("YShader() error with supplied GLSL");
			return;	//	assert(0);
#else
			yplatform::Exit("YShader() error with supplied GLSL");
			return;
#endif
		}
	} while (!success);

	FreeObject(vertex_object);
	FreeObject(fragment_object);
}

/*	FUNCTION:		YShader :: ~YShader
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor.
*/
YShader :: ~YShader()
{
	if (fProgram)
		FreeProgram();
}

/*	FUNCTION:		YShader :: EnableProgram
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Enable shader program
*/
void YShader :: EnableProgram()
{
	if (fProgram)
	{
		glUseProgram(fProgram);
	}
}

/*	FUNCTION:		YShader :: DisableProgram
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Disable shader program
*/
void YShader :: DisableProgram()
{
	if (fProgram)
		glUseProgram(0);
}

/****************************************************
	Manual Shader Management
*****************************************************/

/*	FUNCTION:		YShader :: CreateObjectFromFile
	ARGUMENTS:		filename
					shaderType - can be GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
					source_patch
	RETURN:			handle to shader if success, 0 if error
	DESCRIPTION:	Load shader source code from file, and compile it.
					Allow modification of source if "source_patch" is specified
*/
const GLuint
YShader :: CreateObjectFromFile(const char *filename, const GLenum shaderType, const char *source_patch)
{
	GLuint shader = 0;
	GLint compiled;

	//	Create shader object
	shader = glCreateShader(shaderType);
	if (!shader)
		throw(shaderType);

	//	Read source_code
	size_t len;
	char *source_code = YBuffer_ReadFileToMemory(filename, &len);
	if (!source_code)
	{
		glDeleteShader(shader);
		throw(shaderType);
	}

	//	Transfer source(s) code into shader
	if (source_patch)
	{
		const char *all_sources[3] = {kGLSLVersion, source_patch, source_code};
		glShaderSource(shader, 3, all_sources, 0);
	}
	else
	{
		const char *all_sources[2] = {kGLSLVersion, source_code};
		glShaderSource(shader, 2, all_sources, 0);
	}

	delete [] source_code;
	//	Compile shader
	glCompileShader(shader);

	//	Check for errors
	yplatform::PrintOpenGLError();
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		PrintShaderInfoLog(shader);
		yplatform::Debug("Issue in file: %s\n\n", filename);

		glDeleteShader(shader);
		throw(shaderType);
	}
#if Y_DEBUG_MODE
#if SHADER_VERBOSITY_LEVEL > 0
	else	//	Print info log even though we've successfully compiled the shader 
	{
		if (PrintShaderInfoLog(shader))
			yplatform::Debug("Issue in file: %s\n\n", filename);
	}
#endif
#endif
		
	return shader;
}

/*	FUNCTION:		YShader :: CreateObjectFromMemory
	ARGUMENTS:		source_code
					shaderType - can be GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
					source_patch
	RETURN:			handle to shader if success, 0 if error
	DESCRIPTION:	Similar to YShader::CreateObjectFromFile(), but use source code from memory.
					Allow modification of source if "source_patch" is specified
*/
const GLuint
YShader ::	CreateObjectFromMemory(const char *source_code, const GLenum shaderType, const char *source_patch)
{
	GLuint shader = 0;
	GLint compiled;

	//	Create shader object
	shader = glCreateShader(shaderType);
	if (!shader)
		throw(shaderType);

	//	Transfer source(s) code into shader
	if (source_patch)
	{
		const char *all_sources[3] = {kGLSLVersion, source_patch, source_code};
		glShaderSource(shader, 3, all_sources, 0);
	}
	else
	{
		const char *all_sources[2] = {kGLSLVersion, source_code};
		glShaderSource(shader, 2, all_sources, 0);
	}

	//	Compile shader
	glCompileShader(shader);

	//	Check for errors
	yplatform::PrintOpenGLError();
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		PrintShaderInfoLog(shader);
		glDeleteShader(shader);
		shader = 0;
		throw(shaderType);
	}
#if Y_DEBUG_MODE
#if SHADER_VERBOSITY_LEVEL > 0
	else	//	Print info log even though we've successfully compiled the shader 
		PrintShaderInfoLog(shader);
#endif
#endif

	return shader;
}

/*	FUNCTION:		YShader :: FreeObject
	ARGUMENTS:		shader_id
	RETURN:			none
	DESCRIPTION:	Destroy shader object
*/
void YShader :: FreeObject(const GLuint shader_id)
{
	if (shader_id)
		glDeleteShader(shader_id);
}
	

/*	FUNCTION:		YShader :: CreateProgram
	ARGUMENTS:		shader1				shader object
					shader2				shader object
					attributes			glBindAttribLocation override
					fragdata_binding	glBindFragDataLocation override
	RETURN:			handle to shader program
	DESCRIPTION:	Create program object, attach compiled shader objects, link program
*/
const GLuint
YShader :: CreateProgram(const GLuint shader1, const GLuint shader2,
						 const std::vector<std::string> *attributes,
						 const std::vector<std::string> *fragdata_binding)
{
	assert(fProgram == 0);
	assert((shader1 | shader2) != 0 );
	assert(attributes != NULL);
		
	//	Create program object
	fProgram = glCreateProgram();
	if (!fProgram)
		throw(GL_PROGRAM);

	//	Check attribute limits
	GLint maxVertexAttribs;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
	if ((GLint)attributes->size() > maxVertexAttribs)
	{
		yplatform::Debug("YShader::CreateProgram() - exceeded GL_MAX_VERTEX_ATTRIBS (count=%d, max=%d)\n",
			attributes->size(), maxVertexAttribs);
		glDeleteProgram(fProgram);
		fProgram = 0;
		throw(GL_PROGRAM);
	}

	//	Bind attributes
	GLuint index = 0;
	for (std::vector<std::string>::const_iterator i = attributes->begin();
		i != attributes->end(); ++i)
	{
		glBindAttribLocation(fProgram, index, (*i).c_str());
		PrintProgramInfoLog();
		index++;
	}
	fNumberAttributes = (GLuint)attributes->size();

#if !defined (GL_ES_VERSION_2_0)
	//	fragdata bindings
	if (fragdata_binding)
	{
		index = 0;
		for (std::vector<std::string>::const_iterator i = fragdata_binding->begin();
			i != fragdata_binding->end(); ++i)
		{
			glBindFragDataLocation(fProgram, index, (*i).c_str());
			PrintProgramInfoLog();
			index++;
		}
	}
	else
		glBindFragDataLocation(fProgram, 0, "fragColour");
#endif
    
	//	Attach shader objects
	if (shader1)
		glAttachShader(fProgram, shader1);
	if (shader2)
		glAttachShader(fProgram, shader2);

	//	Link program object
	glLinkProgram(fProgram);

	//	Check for errors
	yplatform::PrintOpenGLError();
	GLint linked;
	glGetProgramiv(fProgram, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		PrintProgramInfoLog();
		glDeleteProgram(fProgram);
		fProgram = 0;
		throw(GL_PROGRAM);
	}
#if Y_DEBUG_MODE
#if SHADER_VERBOSITY_LEVEL > 0
	else	//	Print info log even though we've successfully linked the shaders
		PrintProgramInfoLog();
#endif
#endif

	return fProgram;
}


/*	FUNCTION:		YShader :: FreeProgram
	ARGUMENTS:		program_id
	RETURN:			none
	DESCRIPTION:	Destroy shader object
*/
void
YShader :: FreeProgram()
{
	//	TODO need to detach shaders first
	if (fProgram)
		glDeleteProgram(fProgram);
	fProgram = 0;
}

/****************************************************
	Source management
*****************************************************/

/*	FUNCTION:		YShader :: PrintShaderInfoLog
	ARGUMENTS:		shader object
	RETURN:			true if message printed
	DESCRIPTION:	Print shader debugging info
*/
const bool
YShader :: PrintShaderInfoLog(const GLuint shader)
{
	bool ret = false;
	GLint param;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &param);	// param now stores log length
	if (param > 1)
	{
		char *infoLog = new GLchar [param];
		glGetShaderInfoLog(shader, param, NULL, infoLog);
		glGetShaderiv(shader, GL_SHADER_TYPE, &param);	// param now stores shader type
		yplatform::Debug("[YShader] %s Shader info log\n",
			param == GL_VERTEX_SHADER ? "Vertex" : "Fragment");
		infoLog[0x200] = 0;
		yplatform::Debug("%s\n", infoLog);
		delete [] infoLog;
		ret = true;
	}
	return ret;
}


/*	FUNCTION:		YShader :: PrintProgramInfoLog
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Print shader debugging info
*/
void
YShader :: PrintProgramInfoLog()
{
	assert(fProgram != 0);

	GLint	infoLogLength;
	glGetProgramiv(fProgram, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 1)
	{
		char *infoLog = new GLchar [infoLogLength];
		glGetProgramInfoLog(fProgram, infoLogLength, NULL, infoLog);
		yplatform::Debug("[YShader] Program log\n");
		yplatform::Debug("%s\n", infoLog);
		delete [] infoLog;
	}
}

/*	FUNCTION:		YShader :: ValidateProgram
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Validate GLSL program
*/
void
YShader :: ValidateProgram()
{
	assert(fProgram != 0);
	glValidateProgram(fProgram);

	GLint status;
	glGetProgramiv(fProgram, GL_VALIDATE_STATUS, &status);
	if (status != GL_TRUE)
		PrintProgramInfoLog();
}

/*	FUNCTION:		YShader :: GetUniformLocation
	ARGUMENTS:		name
	RETURN:			location of uniform variables
	DESCRIPTION:	Get location of shader uniform variables
*/
const GLint 
YShader :: GetUniformLocation(const char *name)
{
	GLint location;
	
	location = glGetUniformLocation(fProgram, name);
	if (location == -1)
		yplatform::Debug("YShader::GetUniformLocation(%s) failed (program_id = %d)\n", name, fProgram);

	yplatform::PrintOpenGLError();
	return location;
}

/*	FUNCTION:		YShader :: GetAttributeLocation
	ARGUMENTS:		name
	RETURN:			location of attribute variables
	DESCRIPTION:	Get location of shader attribute variables
*/
const GLint 
YShader :: GetAttributeLocation(const char *name)
{
	GLint location;
	
	location = glGetAttribLocation(fProgram, name);
	if (location == -1)
		yplatform::Debug("YShader::GetAttributeLocation(%s) failed (program_id = %d)\n", name, fProgram);

	yplatform::PrintOpenGLError();
	return location;
}

/*	FUNCTION:		YShader :: PrintToStream
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Display info about shader program
*/
void YShader :: PrintToStream()
{
	assert(fProgram != 0);
	
	GLint count, name_size, array_size;
	GLenum type;
	const GLsizei kBufferSize = 64;
	char buffer[kBufferSize];
	
	yplatform::Debug("[YShader Info.  Program #%d]\n", fProgram);
	
	//	Uniforms
	glGetProgramiv(fProgram, GL_ACTIVE_UNIFORMS, &count);
	yplatform::Debug("[GL_ACTIVE_UNIFORMS]\n");
	glGetProgramiv(fProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &name_size);
	for (int i=0; i < count; i++)
	{
		glGetActiveUniform(fProgram, i, kBufferSize, NULL, &array_size, &type, buffer);
		if (array_size == 1)
			yplatform::Debug("   Loc=%2d %s %s\n", GetUniformLocation(buffer), PrintType(type), buffer);
		else
			yplatform::Debug("   Loc=%2d %s %s (array_size = %d)\n",
						GetUniformLocation(buffer), PrintType(type), buffer, array_size);
	}
	
	//	Attributes
	glGetProgramiv(fProgram, GL_ACTIVE_ATTRIBUTES, &count);
	yplatform::Debug("[GL_ACTIVE_ATTRIBUTES]\n");
	glGetProgramiv(fProgram, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &name_size);
	for (int i=0; i < count; i++)
	{
		glGetActiveAttrib(fProgram, i, kBufferSize, NULL, &array_size, &type, buffer);
		if (array_size == 1)
			yplatform::Debug("   Loc=%2d %s %s\n", GetAttributeLocation(buffer), PrintType(type), buffer);
		else
			yplatform::Debug("   Loc=%2d %s %s (array_size = %d)\n",
						GetAttributeLocation(buffer), PrintType(type), buffer, array_size);
	}
}

/*	FUNCTION:		YShader :: PrintType
	ARGUMENTS:		type
	RETURN:			text description of type
	DESCRIPTION:	Called by PrintToStream() to display GLenum
*/
const char *
YShader :: PrintType(GLenum type)
{
	switch (type)
	{
		case GL_FLOAT:				return "[GL_FLOAT]       ";
		case GL_FLOAT_VEC2:			return "[GL_FLOAT_VEC2]  ";
		case GL_FLOAT_VEC3:			return "[GL_FLOAT_VEC3]  ";
		case GL_FLOAT_VEC4:			return "[GL_FLOAT_VEC4]  ";
		case GL_INT:				return "[GL_INT]         ";
		case GL_INT_VEC2:			return "[GL_INT_VEC2]    ";
		case GL_INT_VEC3:			return "[GL_INT_VEC3]    ";
		case GL_INT_VEC4:			return "[GL_INT_VEC4]    ";
		case GL_BOOL:				return "[GL_BOOL]        ";
		case GL_BOOL_VEC2:			return "[GL_BOOL_VEC2]   ";
		case GL_BOOL_VEC3:			return "[GL_BOOL_VEC3]   ";
		case GL_BOOL_VEC4:			return "[GL_BOOL_VEC4]   ";
		case GL_FLOAT_MAT2:			return "[GL_FLOAT_MAT2]  ";
		case GL_FLOAT_MAT3:			return "[GL_FLOAT_MAT3]  ";
		case GL_FLOAT_MAT4:			return "[GL_FLOAT_MAT4]  ";
		case GL_SAMPLER_2D:			return "[GL_SAMPLER_2D]  ";
		case GL_SAMPLER_CUBE:		return "[GL_SAMPLER_CUBE]";
		default:
			break;
	}
	
	yplatform::Debug("YShader::PrintType(0x%x) - unknown GLenum\n", type);
	return "unknown";
}

/****************************
	Minimal Shader
*****************************/
YShader *	YMinimalShader :: sShader = nullptr;
int			YMinimalShader :: sInstanceCount = 0;
GLint		YMinimalShader :: sLocation_uTransform = -1;
GLint		YMinimalShader :: sLocation_uTextureUnit0 = -1;

static const char *kVertexShader = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec2			aTexture0;\
	out vec2		vTexCoord0;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);\
		vTexCoord0 = aTexture0;\
	}";

static const char *kFragmentShader = "\
	uniform sampler2D	uTextureUnit0;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour = texture(uTextureUnit0, vTexCoord0);\
	}";

/*	FUNCTION:		YMinimalShader :: YMinimalShader
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YMinimalShader :: YMinimalShader()
{
	if (++sInstanceCount == 1)
	{
		assert(sShader == nullptr);
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		sShader = new YShader(&attributes, kVertexShader, kFragmentShader);
		sLocation_uTransform = sShader->GetUniformLocation("uTransform");
		sLocation_uTextureUnit0 = sShader->GetUniformLocation("uTextureUnit0");
	}
}

/*	FUNCTION:		YMinimalShader :: ~YMinimalShader
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
YMinimalShader :: ~YMinimalShader()
{
	if (--sInstanceCount == 0)
	{
		delete sShader;
		sShader = nullptr;
	}
}

/*	FUNCTION:		YMinimalShader :: Render
	ARGUMENTS:		delta_time
	RETURN:			n/a
	DESCRIPTION:	Activate shader
*/
void YMinimalShader :: Render(float delta_time)
{
	sShader->EnableProgram();
	glUniformMatrix4fv(sLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
	glUniform1i(sLocation_uTextureUnit0, 0);
}

};	//	namespace yrender

