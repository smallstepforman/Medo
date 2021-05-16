uniform sampler2D		uTextureUnit0;
uniform sampler2D		uTextureUnit1;
uniform vec2					uCenter;
uniform float					uTime;
uniform float					uWidth;

in vec2				vTexCoord0;
out vec4			fFragColour;

void main()
{
	vec4 tx0 = texture(uTextureUnit0, vTexCoord0);
	vec4 tx1 = texture(uTextureUnit1, vTexCoord0);
	
	vec2 uv = vTexCoord0;
	uv -= uCenter;
	float dist = sqrt(dot(uv, uv));
	
	//	Iris
	float t = smoothstep(uTime+uWidth, uTime-uWidth, dist);
	fFragColour = mix(tx1, tx0,t);	
}
