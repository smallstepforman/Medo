/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2010, ZenYes Pty Ltd
	DESCRIPTION:	Default shader for drawing FTGL textured fonts
*/

uniform mat4		uTransform;

in vec3		aPosition;
in vec2		aTexture0;

out vec2		vTexCoord0;

/*	FUNCTION:		main	 Y_FTGL_Font.vert
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Vertex shader
*/
void main(void)
{
	gl_Position = uTransform * vec4(aPosition, 1.0);
	
	vTexCoord0 = aTexture0;
}
