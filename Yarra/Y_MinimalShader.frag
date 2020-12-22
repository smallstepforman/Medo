/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2013, ZenYes Pty Ltd
	DESCRIPTION:	Minimal shader
*/

uniform sampler2D	uTextureUnit0;
in vec2				vTexCoord0;
out vec4			fragColour;

/*	FUNCTION:		main	 Y_MinimalShader.frag
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Fragment shader
*/
void main(void)
{
	//fragColour = texture(uTextureUnit0, vTexCoord0).bgra;
	fragColour = texture(uTextureUnit0, vTexCoord0);
}
