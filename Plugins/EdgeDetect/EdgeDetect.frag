uniform sampler2D		iChannel0;
uniform float			uIntesity;
uniform int				uColour;
uniform vec2			iResolution;

in vec2				vTexCoord0;
out vec4			fFragColour;

// Basic edge detection via convolution
// Ken Slade - ken.slade@gmail.com
// at https://www.shadertoy.com/view/ldsSWr

// Based on original Sobel shader by:
// Jeroen Baert - jeroen.baert@cs.kuleuven.be (www.forceflow.be)
// at https://www.shadertoy.com/view/Xdf3Rf

// Sobel masks (see http://en.wikipedia.org/wiki/Sobel_operator)
const mat3 sobelKernelX = mat3(1.0, 0.0, -1.0,
							   2.0, 0.0, -2.0,
							   1.0, 0.0, -1.0);

const mat3 sobelKernelY = mat3(-1.0, -2.0, -1.0,
							   0.0, 0.0, 0.0,
							   1.0, 2.0, 1.0);

//performs a convolution on an image with the given kernel
float convolve(mat3 kernel, mat3 image) {
	float result = 0.0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			result += kernel[i][j]*image[i][j];
		}
	}
	return result;
}

//helper function for colorEdge()
float convolveComponent(mat3 kernelX, mat3 kernelY, mat3 image) {
	vec2 result;
	result.x = convolve(kernelX, image);
	result.y = convolve(kernelY, image);
	return clamp(length(result), 0.0, 255.0);
}

//finds edges where fragment intensity changes from a higher value to a lower one (or
//vice versa).
vec4 edge(float stepx, float stepy, vec2 center, mat3 kernelX, mat3 kernelY){
	// get samples around pixel
	mat3 image = mat3(length(texture(iChannel0,center + vec2(-stepx,stepy)).rgb),
					  length(texture(iChannel0,center + vec2(0,stepy)).rgb),
					  length(texture(iChannel0,center + vec2(stepx,stepy)).rgb),
					  length(texture(iChannel0,center + vec2(-stepx,0)).rgb),
					  length(texture(iChannel0,center).rgb),
					  length(texture(iChannel0,center + vec2(stepx,0)).rgb),
					  length(texture(iChannel0,center + vec2(-stepx,-stepy)).rgb),
					  length(texture(iChannel0,center + vec2(0,-stepy)).rgb),
					  length(texture(iChannel0,center + vec2(stepx,-stepy)).rgb));
 	vec2 result;
	result.x = convolve(kernelX, image);
	result.y = convolve(kernelY, image);
	
    float color = clamp(length(result), 0.0, 255.0);
    return vec4(color);
}

//Colors edges using the actual color for the fragment at this location
vec4 trueColorEdge(float stepx, float stepy, vec2 center, mat3 kernelX, mat3 kernelY) {
	vec4 edgeVal = edge(stepx, stepy, center, kernelX, kernelY);
	return edgeVal * texture(iChannel0,center);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	vec2 uv = fragCoord.xy / iResolution.xy;
	vec4 color = texture(iChannel0, uv.xy);

	if (uColour > 0)
		fragColor = trueColorEdge(uIntesity/iResolution[0], uIntesity/iResolution[1],
								uv,
								sobelKernelX, sobelKernelY);
	else
		fragColor = edge(uIntesity/iResolution[0], uIntesity/iResolution[1],
								uv,
								sobelKernelX, sobelKernelY);
}

void main()
{
	mainImage(fFragColour, vTexCoord0*iResolution);
}
