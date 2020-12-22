/*

TT Highlights and Shadows - Fragment Shader

version v1.2

Coded for Isadora by Sarah Whitley

Inspired by code in GPUImage by Brad Larsen
see https://github.com/BradLarson/GPUImage

*/

uniform sampler2D		uTexture0;
uniform float			uShadow;
uniform float			uHighlight;

in vec2				vTexCoord0;
out vec4			fFragColour;

//const vec3 luminanceWeighting = vec3(0.2126, 0.7152, 0.0722);
const vec3 luminanceWeighting = vec3(0.3, 0.3, 0.3);

void main()
 {
	vec4 source = texture(uTexture0, vTexCoord0);
	float luminance = dot(source.rgb, luminanceWeighting);
    
	float shadow = clamp((pow(luminance, 1.0/(uShadow+1.0)) + (-0.76)*pow(luminance, 2.0/(uShadow+1.0))) - luminance, 0.0, 1.0);
	float highlight = clamp((1.0 - (pow(1.0-luminance, 1.0/(2.0-uHighlight)) + (-0.8)*pow(1.0-luminance, 2.0/(2.0-uHighlight)))) - luminance, -1.0, 0.0);
	vec3 result = vec3(0.0, 0.0, 0.0) + ((luminance + shadow + highlight) - 0.0) * ((source.rgb - vec3(0.0, 0.0, 0.0))/(luminance - 0.0));
    

	fFragColour = vec4(result.rgb, source.a);
}
