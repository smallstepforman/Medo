uniform sampler2D		uTextureUnit0;
uniform float time;

in vec2				vTexCoord0;
out vec4			fFragColour;

// Source: 
// https://tympanus.net/codrops/2016/05/03/animated-heat-distortion-effects-webgl/

uniform float haze_speed; // 0.03
uniform float haze_frequency; // 100.0
uniform float haze_amplitude; // 0.004


vec3 haze(vec2 uv)
{
  float distortion = sin(uv.y*haze_frequency+time*haze_speed)*haze_amplitude;
  return texture(uTextureUnit0,vec2(uv.x+distortion, uv.y)).rgb;
}


void main() 
{ 
  vec3 fragcol = haze(vTexCoord0);
	fFragColour = vec4(fragcol, 1.0);
}
