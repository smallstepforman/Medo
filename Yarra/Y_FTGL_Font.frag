/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2010, ZenYes Pty Ltd
	DESCRIPTION:	Default shader for drawing FTGL textured fonts
*/

uniform vec4		uColour;
uniform sampler2D	uTextureUnit0;

in vec2		vTexCoord0;

out vec4 fragColour;

/*	FUNCTION:		main	 Y_FTGL_Font.frag
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Fragment shader.
					FTGL originally used the GL_ALPHA format for glTexImage2D(),
					but this is not supported in the CORE OpenGL profile.
					Instead, we'll use GL_RED format (any single byte format will do),
					from which we will extract the alpha component.
					The final fragment colour is a combination of the desired colour (uColour)
					and alpha information from the FTGL font texture map.
*/
void main(void)
{
	fragColour = uColour * vec4(vec3(1.0), texture(uTextureUnit0, vTexCoord0).r);
}
