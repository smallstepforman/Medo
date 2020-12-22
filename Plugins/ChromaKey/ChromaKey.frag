uniform sampler2D		uTexture0;
uniform float			uThreshold;
uniform float			uSmoothing;
uniform vec4			uChromaColour;

in vec2				vTexCoord0;
out vec4			fFragColour;

void main()
 {
	vec4 textureColor = texture(uTexture0, vTexCoord0) ;

	float maskY = 0.2989 * uChromaColour.r + 0.5866 * uChromaColour.g + 0.1145 * uChromaColour.b;
	float maskCr = 0.7132 * (uChromaColour.r - maskY);
	float maskCb = 0.5647 * (uChromaColour.b - maskY);

	float Y = 0.2989 * textureColor.r + 0.5866 * textureColor.g + 0.1145 * textureColor.b;
	float Cr = 0.7132 * (textureColor.r - Y);
	float Cb = 0.5647 * (textureColor.b - Y);
     
	//     float blendValue = 1.0 - smoothstep(uThreshold - uSmoothing, uThreshold , abs(Cr - maskCr) + abs(Cb - maskCb));
	float blendValue = smoothstep(uThreshold, uThreshold + uSmoothing, distance(vec2(Cr, Cb), vec2(maskCr, maskCb)));
	fFragColour = vec4(textureColor.rgb, textureColor.a * blendValue);
}