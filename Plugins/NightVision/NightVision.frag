uniform sampler2D		uTextureUnit0;
uniform vec2					resolution;
uniform float					time;

in vec2				vTexCoord0;
out vec4			fFragColour;

// Source:
// https://github.com/crosire/reshade-shaders/blob/master/Shaders/NightVision.fx

float hash(float n) 
{ 
  return fract(sin(n)*43758.5453123); 
}

float mod(float x, float y)
{
  return x - y * floor (x/y);
}

vec3 nightvision(vec2 uv, vec3 color)
{ 
  vec2 p = uv;
  float t = time + 1.0;
  
  vec2 u = p * 2. - 1.;
  vec2 n = u * vec2(resolution.x / resolution.y, 1.0);
  vec3 c = color;

  // flicker, grain, vignette, fade in
  c += sin(hash(time*0.001)) * 0.01;
  c += hash((hash(n.x) + n.y) * t*0.001) * 0.5;
  c *= smoothstep(length(n * n * n * vec2(0.0, 0.0)), 1.0, 0.4);
  c *= smoothstep(0.01, 1.0, t) * 1.5;
  
  c = dot(c, vec3(0.2126, 0.7152, 0.0722)) * vec3(0.2, 1.5 - hash(t*0.001) * 0.1, 0.4);
  return c;
}

void main()
{
	vec3 colour = texture(uTextureUnit0, vTexCoord0).rgb;
	vec3 fragcol = nightvision(vTexCoord0, colour);
	fFragColour = vec4(fragcol, 1.0);
}
