uniform sampler2D		uTextureUnit0;
uniform sampler2D		uTextureUnit1;
uniform float			iTime;
uniform float			uIntensity;

in vec2				vTexCoord0;
out vec4			fFragColour;

float Hash( vec2 p)
{
	vec3 p2 = vec3(p.xy,1.0);
    return fract(sin(dot(p2,vec3(37.1,61.7, 12.4)))*3758.5453123);
}

float noise(in vec2 p)
{
    vec2 i = floor(p);
	vec2 f = fract(p);
	f *= f * (3.0-2.0*f);

    return mix(mix(Hash(i + vec2(0.,0.)), Hash(i + vec2(1.,0.)),f.x),
		mix(Hash(i + vec2(0.,1.)), Hash(i + vec2(1.,1.)),f.x),
		f.y);
}

float fbm(vec2 p) 
{
	float v = 0.0;
	v += noise(p*1.)*.5;
	v += noise(p*2.)*.25;
	v += noise(p*4.)*.125;
	return v;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord;
	vec3 tgt = texture(uTextureUnit0, uv).rgb;
	vec3 src = texture(uTextureUnit1, uv).rgb;
	vec3 col = src;
	uv.x -= 1.5;
	
	float ctime = iTime*1.45;
	
	// burn
	float d = uv.x+uv.y*0.5 + 0.5*fbm(uv*15.1) + ctime*1.3;
	if (d >0.35)
		col = clamp(col-(d-0.35)*10.,0.0,1.0);
	if (d >uIntensity)
	{
		if (d < 0.5 )
			col += (d-0.4)*33.0*0.5*(0.0+noise(100.*uv+vec2(-ctime)))*vec3(0.0,0.5,1.5);
		else
			col += tgt;
	}
	fragColor = vec4(col, 1.0);
}

void main()
{
	mainImage(fFragColour, vTexCoord0);
}
