uniform sampler2D		uTextureUnit0;
uniform vec2					resolution;
uniform float					radial_blur;
uniform float					radial_bright;
uniform int						interpolate;
uniform float					interval;

in vec2				vTexCoord0;
out vec4			fFragColour;

vec2 radial_origin = vec2(0.5, 0.5);

vec3 radialblur(vec2 texcoord)
{
  vec2 radial_size = vec2(1.0/resolution.x, 1.0/resolution.y);
  vec2 uv = texcoord;
  vec3 sum = vec3(0.0, 0.0, 0.0);
  uv += radial_size * 0.5 - radial_origin;
  
  float amount = radial_blur;
  float brightness = radial_bright;
  
  if (interpolate == 1)
  {
  	amount *= interval;
  	brightness = mix(1.0, radial_bright, interval);
  }
  
  for (int i = 0; i < 32; i++) 
  {
    float scale = 1.0 - amount * (float(i) / 11.0);
    sum += texture(uTextureUnit0, uv * scale + radial_origin).rgb;
  }

  return sum / 32.0 * brightness;
}

void main()
{
	vec3 fragcol = radialblur(vTexCoord0);
	fFragColour = vec4(fragcol, 1.0);
}
