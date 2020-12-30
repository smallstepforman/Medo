uniform sampler2D		uTextureUnit0;
uniform sampler2D		uTextureUnit1;
uniform float					uMidPoint;
uniform float					uOffsetTex0;
uniform float					uOffsetTex1;

uniform float					uTime;
uniform int						uInterpolateIn;
uniform float					uInterpolateInThreshold;
uniform int						uInterpolateOut;
uniform float					uInterpolateOutThreshold;

in vec2				vTexCoord0;
out vec4				fFragColour;

const float kBlackWidth = 0.002;

void main()
{
	float midpoint = uMidPoint;
	if ((uInterpolateIn > 0) &&  (uTime < uInterpolateInThreshold + kBlackWidth))
	{
		float t = uTime / uInterpolateInThreshold;
		midpoint = mix(1.0, uMidPoint, t);
		
		if ((vTexCoord0.x > midpoint - kBlackWidth) && (vTexCoord0.x < midpoint + kBlackWidth))
		{
			fFragColour = vec4(0,0,0,1);
			return;
		}
			
		if (vTexCoord0.x < midpoint)
		{
			fFragColour = texture(uTextureUnit0, vec2(vTexCoord0.x + uOffsetTex0*t, vTexCoord0.y));
		}
		else
		{
			fFragColour = texture(uTextureUnit1, vec2((vTexCoord0.x - midpoint) + uOffsetTex1*t, vTexCoord0.y));
		}
	}
	else if ((uInterpolateOut > 0) &&  (uTime > uInterpolateOutThreshold - kBlackWidth))
	{
		float t = (uTime-uInterpolateOutThreshold) / (1.0 - uInterpolateOutThreshold);
		midpoint = mix(uMidPoint, 0.0, t);
		
		if ((vTexCoord0.x > midpoint - kBlackWidth) && (vTexCoord0.x < midpoint + kBlackWidth))
		{
			fFragColour = vec4(0,0,0,1);
			return;
		}
			
		if (vTexCoord0.x < midpoint)
		{
			fFragColour = texture(uTextureUnit0, vec2(vTexCoord0.x + uOffsetTex0*(1.0-t), vTexCoord0.y));
		}
		else
		{
			fFragColour = texture(uTextureUnit1, vec2((vTexCoord0.x - midpoint) + uOffsetTex1*(1.0-t), vTexCoord0.y));
		}	
	}
	else
	{
		if ((vTexCoord0.x > midpoint - kBlackWidth) && (vTexCoord0.x < midpoint + kBlackWidth))
		{
			fFragColour = vec4(0,0,0,1);
			return;
		}
		
		if (vTexCoord0.x < midpoint)
		{
			fFragColour = texture(uTextureUnit0, vec2(vTexCoord0.x + uOffsetTex0, vTexCoord0.y));
		}
		else
		{
			fFragColour = texture(uTextureUnit1, vec2((vTexCoord0.x - midpoint) + uOffsetTex1, vTexCoord0.y));
		}
	}
}
