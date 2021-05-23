//	From GLSL-Color_Spaces (https:://github.com/tobspr/GLSL-Color-Spaces)

uniform sampler2D		uTexture0;
uniform int						uDirection;
uniform float					uGamma;

in vec2				vTexCoord0;
out vec4			fFragColour;

const float SRGB_ALPHA = 0.055;

float srgb_linear(float c)
{
	if (c <= 0.04045)
		return c/12.92;
	else
		return pow((c + SRGB_ALPHA) / (1.0 + SRGB_ALPHA), 2.4);	
}
float linear_srgb(float c)
{
	if (c <= 0.0031308)
		return 12.92*c;
	else
		return (1.0 + SRGB_ALPHA) * pow(c, 1.0/2.f) - SRGB_ALPHA;
}
float gamma_linear(float c)
{
	return pow(c, uGamma);	
}
float linear_gamma(float c)
{
	return pow(c, 1.0/uGamma);	
}

void main()
 {
	vec4 colour = texture(uTexture0, vTexCoord0);
	if (uDirection == 0)
 		fFragColour = vec4(srgb_linear(colour.r), srgb_linear(colour.g), srgb_linear(colour.b), colour.a);
 	else if (uDirection == 1)
 		fFragColour = vec4( linear_srgb(colour.r), linear_srgb(colour.g), linear_srgb(colour.b), colour.a);	
 	else if (uDirection == 2)
 		fFragColour = vec4( gamma_linear(colour.r), gamma_linear(colour.g), gamma_linear(colour.b), colour.a);	
 	else //if (uDirection == 3)
 		fFragColour = vec4( linear_gamma(colour.r), linear_gamma(colour.g), linear_gamma(colour.b), colour.a);	
}
