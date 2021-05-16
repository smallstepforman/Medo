uniform sampler2D		uTextureUnit0;
uniform vec2					uCenter;
uniform float					uRadius;
uniform vec2					uAspect;
uniform vec4					uColour;
uniform float					uWidth;
uniform bool					uFilled;
uniform bool					uAnimated;
uniform float					uAnimatedInterval;
uniform float					uTime;

in vec2				vTexCoord0;
out vec4			fFragColour;

void main()
{
	vec4 tx0 = texture(uTextureUnit0, vTexCoord0);
	
	vec2 uv = vTexCoord0;
	uv -= uCenter;
	uv /= uAspect;
	float dist = sqrt(dot(uv, uv));
	
	if (uAnimated && (uTime < uAnimatedInterval))
	{
		float dt = uTime/uAnimatedInterval;
		if (dt < vTexCoord0.x)
		{
			fFragColour = tx0;
			return;
		}
	}
	
	if (uFilled)
	{
		//	Disc
		float t = 1.0 - smoothstep(uRadius+uWidth, uRadius-uWidth, dist);
		fFragColour = mix(uColour, tx0, t);	
	}
	else
	{
		//	Circle
		float t = 1.0 + smoothstep(uRadius, uRadius + uWidth, dist) - smoothstep(uRadius - uWidth, uRadius, dist);
		fFragColour = mix(uColour, tx0, t);	
	}
}
