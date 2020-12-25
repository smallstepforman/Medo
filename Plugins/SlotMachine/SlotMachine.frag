uniform sampler2D		uTextureUnit0;
uniform sampler2D		uTextureUnit1;
uniform float					uTime;

in vec2				vTexCoord0;
out vec4			fFragColour;

float mod_func(float a, float b)
{
	return a - (b * floor(a/b))	;
}

#ifndef PI
#define PI 3.141592653589793
#endif

float sineInOut(float t) {
  return -0.5 * (cos(PI * t) - 1.0);
}

void main()
{
	vec2 uv = vTexCoord0;
	if (uv.x < 0.20)
	{
		float t = uTime/0.4;
		if (t < 1.0)
		{
			float y = 3.0*sineInOut(t); 
			if (y < 2.0 || mod_func(uv.y + y, 2.0) < 1.0)
				fFragColour = texture(uTextureUnit0, vec2(uv.x, uv.y + y));
			else
				fFragColour = texture(uTextureUnit1, vec2(uv.x, uv.y + y));
		}
		else
			fFragColour = texture(uTextureUnit1, vec2(uv.x, uv.y));
	}
	else if (uv.x < 0.40)
	{
		float t = uTime/0.55;
		if (t < 1.0)
		{
			float y = 5.0*sineInOut(t);
			if (mod_func(uv.y + y, 2.0) < 1.0)
				fFragColour = texture(uTextureUnit0, vec2(uv.x, uv.y + y));
			else
				fFragColour = texture(uTextureUnit1, vec2(uv.x, uv.y + y));
		}
		else
			fFragColour = texture(uTextureUnit1, vec2(uv.x, uv.y));
	}
	else if (uv.x < 0.60)
	{
		float t = uTime/0.70;
		if (t < 1.0)
		{
			float y = 7.0*sineInOut(t); 
			if (y < 2.0 || mod_func(uv.y + y, 2.0) < 1.0)
				fFragColour = texture(uTextureUnit0, vec2(uv.x, uv.y + y));
			else
				fFragColour = texture(uTextureUnit1, vec2(uv.x, uv.y + y));
		}
		else
			fFragColour = texture(uTextureUnit1, vec2(uv.x, uv.y));
	}
	else if (uv.x < 0.80)
	{
		float t = uTime/0.85;
		if (t < 1.0)
		{
			float y = 9.0*sineInOut(t); 
			if (mod_func(uv.y + y, 2.0) < 1.0)
				fFragColour = texture(uTextureUnit0, vec2(uv.x, uv.y + y));
			else
				fFragColour = texture(uTextureUnit1, vec2(uv.x, uv.y + y));
		}
		else
			fFragColour = texture(uTextureUnit1, vec2(uv.x, uv.y));
	}
	else
	{
		float t = uTime;
		if (t < 1.0)
		{
			float y = 11.0*sineInOut(t);
			if (y < 2.0 || mod_func(uv.y + y, 2.0) < 1.0)
				fFragColour = texture(uTextureUnit0, vec2(uv.x, uv.y + y));
			else
				fFragColour = texture(uTextureUnit1, vec2(uv.x, uv.y + y));
		}
		else
			fFragColour = texture(uTextureUnit1, vec2(uv.x, uv.y));
	}
}
