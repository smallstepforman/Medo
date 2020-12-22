uniform sampler2D		uTextureUnit0;
uniform vec2			resolution;
uniform float			pixelate_size_x;
uniform float			pixelate_size_y;
uniform float			time;
uniform int				interpolate;

in vec2				vTexCoord0;
out vec4			fFragColour;

vec3 pixelate(vec2 uv)
{
	float t = 1.0;
	if (interpolate == 1)
		t = time;
	
	float dx = t*pixelate_size_x * (1./resolution.x);
  	float dy = t*pixelate_size_y * (1./resolution.y);
	
	vec2 coord = vec2(dx*floor(uv.x/dx), dy*floor(uv.y/dy));
  return texture(uTextureUnit0, coord).rgb;
}

void main() 
{ 
  fFragColour = vec4(pixelate(vTexCoord0), 1.0);
}
