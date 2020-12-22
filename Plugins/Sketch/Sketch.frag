uniform sampler2D		tex0;
uniform float			uEdgeSlope;
uniform float			uPower;
uniform vec2			uResolution;

in vec2				vTexCoord0;
out vec4			fFragColour;

vec3 cartoon(vec2 texcoord, vec3 color)
{
  vec3 coefLuma = vec3(0.2126, 0.7152, 0.0722);
  vec2 PixelSize = vec2(1.0/uResolution.x, 1.0/uResolution.y);
  float EdgeSlope = uEdgeSlope;
  float Power = uPower;

  float diff1 = dot(coefLuma, texture(tex0, texcoord + PixelSize).rgb);
  diff1 = dot(vec4(coefLuma, -1.0), vec4(texture(tex0, texcoord - PixelSize).rgb , diff1));
  float diff2 = dot(coefLuma, texture(tex0, texcoord + PixelSize * vec2(1.0, -1.0)).rgb);
  diff2 = dot(vec4(coefLuma, -1.0), vec4(texture(tex0, texcoord + PixelSize * vec2(-1.0, 1.0)).rgb , diff2));

  float edge = dot(vec2(diff1, diff2), vec2(diff1, diff2));

  //return saturate(pow(edge, EdgeSlope) * -Power + color);
  return clamp(pow(edge, EdgeSlope) * -Power + color, 0.0, 1.0);
}

void main()
 {
	 vec3 fragcol = vec3(0.0);
  //vec2 src_size = textureSize(tex0 , 0);
  //vec2 uv = gl_FragCoord.xy / src_size;
  vec2 uv = vTexCoord0;

  vec3 color = texture(tex0, uv).rgb;

 fragcol = cartoon(uv, color);
 

  fFragColour = vec4(fragcol, 1.0);
}
