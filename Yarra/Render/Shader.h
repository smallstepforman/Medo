/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	A YShader manages GLSL programs.
*/

#ifndef __YARRA_SHADER_H__
#define __YARRA_SHADER_H__

#ifndef _YARRA_PLATFORM_H_
#include "Yarra/Platform.h"
#endif

#ifndef _GLIBCXX_STRING
#include <string>
#endif

#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif

namespace yrender
{
	
/****************************************************************
	YShader.  The constructor uses glBindAttribLocation() to ensure
	that the indices used in glVertexAttribPointer() line up.
	To create a YShader, do the following:
		std::vector<std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture"};
		YShader("vertex_shader.vs", "fragement_shader.fs", &attributes);
	Obviously, the attribute indices will correspond to the attribute order.
	The fragdata binding defaults to "out vec4 fragColour"
*****************************************************************/
class YShader
{
public:
	
						YShader();
						YShader(const char *vertex_shader_source_file,
								const char *fragment_shader_source_file,
								const std::vector<std::string> *attribute_binding,
								const std::vector<std::string> *fragdata_binding = 0,
								const char *vertex_shader_patch = 0,
								const char *fragment_shader_patch = 0);
						YShader(const std::vector<std::string> *attribute_binding,
								const char *vertex_shader_source_text,
								const char *fragment_shader_source_text,
								const std::vector<std::string> *fragdata_binding = 0);
						~YShader();

	void				EnableProgram();
	void				DisableProgram();
	void				ValidateProgram();		// caution - slow
	void				PrintToStream();
		
	const GLint			GetUniformLocation(const char *name);
	const GLint			GetAttributeLocation(const char *name);
	
	//	Manual shader management
	const GLuint		CreateObjectFromFile(const char *filename, const GLenum shaderType,
									const char *source_patch = 0);	
	const GLuint		CreateObjectFromMemory(const char *source_code, const GLenum shaderType,
									const char *source_patch = 0);	
	const GLuint		CreateProgram(const GLuint shader1, const GLuint shader2,
									const std::vector<std::string> *attribute_binding,
									const std::vector<std::string> *fragdata_binding = 0);
	void				FreeObject(const GLuint id);
	void				FreeProgram();
	const GLuint		GetProgram() {return fProgram;}
	
private:
	char				*LoadSourceFile(const char *filename);
	const bool			PrintShaderInfoLog(const GLuint obj);
	void				PrintProgramInfoLog();
	const char			*PrintType(GLenum type);

	GLuint				fProgram;
	GLuint				fNumberAttributes;	
};

/***********************************
	YShaderNode
************************************/
class YShaderNode : public YSceneNode
{
public:
	virtual			~YShaderNode() {}
	virtual void	Render(float delta_time) = 0;
};

/***********************************
	YMinimalShader
************************************/
class YMinimalShader : public YShaderNode
{
public:
			YMinimalShader();
			~YMinimalShader();
	void	Render(float delta_time);

private:
	static YShader		*sShader;
	static int			sInstanceCount;
	static GLint		sLocation_uTransform;
	static GLint		sLocation_uTextureUnit0;
};

};	//	namespace yrender

#endif	//#ifndef __YARRA_SHADER_H__

