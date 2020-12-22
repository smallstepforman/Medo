uniform sampler2D		uTexture0;
uniform float			uMinInput;
uniform float			uGamma;
uniform float			uMaxInput;

in vec2				vTexCoord0;
out vec4			fFragColour;

vec3 gammaCorrect(vec3 color, float gamma)
{
    return pow(color, vec3(1.0/gamma));
}

vec3 levelRange(vec3 color, float minInput, float maxInput)
{
    return min(max(color - vec3(minInput), vec3(0.0)) / (vec3(maxInput) - vec3(minInput)), vec3(1.0));
}

vec3 finalLevels(vec3 color, float minInput, float gamma, float maxInput)
{
    return gammaCorrect(levelRange(color, minInput, maxInput), gamma);
}

void main()
 {
	vec4 textureColor = texture(uTexture0, vTexCoord0);
 	fFragColour = vec4(finalLevels(textureColor.rgb, uMinInput, uGamma, uMaxInput), textureColor.a);
}