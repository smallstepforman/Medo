uniform sampler2D		uTextureUnit0;
uniform sampler2D		uTextureUnit1;
uniform vec2			iResolution;
uniform float			iTime;
uniform float			count;
uniform float			smoothness;

in vec2				vTexCoord0;
out vec4			fFragColour;

#define inputImageTexture uTextureUnit1
#define inputImageTexture2 uTextureUnit0
#define texture(a,b) texture(a,b)
#define textureCoordinate (fragCoord.xy / iResolution.xy)
#define textureCoordinate2 (fragCoord.xy / iResolution.xy)

#define progress fract(iTime)
#define ratio (iResolution.x / iResolution.y)

#define REV_VEC2(p) vec2(p.x, 1.0 - p.y)
#define getFromColor(coord) texture2D(inputImageTexture, REV_VEC2(coord))
#define getToColor(coord) texture2D(inputImageTexture2, REV_VEC2(coord))

highp vec4 transition (highp vec2 p) {
    highp float pr = smoothstep(-smoothness, 0.0, p.x - progress * (1.0 + smoothness));
    highp float s = step(pr, fract(count * p.x));
    return mix(getFromColor(p), getToColor(p), s);
}

void main()
{
	fFragColour = transition(vec2(vTexCoord0.x, 1.0-vTexCoord0.y));
}
