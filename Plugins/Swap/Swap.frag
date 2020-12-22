uniform sampler2D		uTextureUnit0;
uniform sampler2D		uTextureUnit1;
uniform vec2			iResolution;
uniform float			iTime;
uniform float			reflection;
uniform float			project_y1;
uniform float			project_y2;

in vec2				vTexCoord0;
out vec4			fFragColour;

#define from uTextureUnit1
#define to uTextureUnit0
#define resolution (iResolution.xy)

float progress;
float perspective = .2;
float depth = 3.;
 
const vec4 black = vec4(0.0, 0.0, 0.0, 1.0);
const vec2 boundMin = vec2(0.0, 0.0);
const vec2 boundMax = vec2(1.0, 1.0);
 
bool inBounds (vec2 p) {
  return all(lessThan(boundMin, p)) && all(lessThan(p, boundMax));
}
 
vec2 project (vec2 p) {
  //return p * vec2(1.0, -1.2) + vec2(0.0, -0.02);
  return p * vec2(1.0, project_y1) + vec2(0.0, project_y2);
}
 
vec4 bgColor (vec2 p, vec2 pfr, vec2 pto) {
  vec4 c = black;
  pfr = project(pfr);
  if (inBounds(pfr)) {
    c += mix(black, texture(from, vec2(pfr.x, 1.0-pfr.y)), reflection * mix(1.0, 0.0, pfr.y));
  }
  pto = project(pto);
  if (inBounds(pto)) {
    c += mix(black, texture(to, vec2(pto.x, 1.0-pto.y)), reflection * mix(1.0, 0.0, pto.y));
  }
  return c;
}
 
void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
  progress = sin(iTime*.5)*.5+.5;
  vec2 p = fragCoord.xy / resolution.xy;
  //if (iMouse.z>0.) progress = iMouse.x/iResolution.x;
  progress = iTime;

  vec2 pfr, pto = vec2(-1.);
 
  float size = mix(1.0, depth, progress);
  float persp = perspective * progress;
  pfr = (p + vec2(-0.0, -0.5)) * vec2(size/(1.0-perspective*progress), size/(1.0-size*persp*p.x)) + vec2(0.0, 0.5);
 
  size = mix(1.0, depth, 1.-progress);
  persp = perspective * (1.-progress);
  pto = (p + vec2(-1.0, -0.5)) * vec2(size/(1.0-perspective*(1.0-progress)), size/(1.0-size*persp*(0.5-p.x))) + vec2(1.0, 0.5);
 
  bool fromOver = progress < 0.5;
 
  if (fromOver) {
    if (inBounds(pfr)) {
      fragColor = texture(from, pfr);
    }
    else if (inBounds(pto)) {
      fragColor = texture(to, pto);
    }
    else {
      fragColor = bgColor(p, pfr, pto);
    }
  }
  else {
    if (inBounds(pto)) {
      fragColor = texture(to, pto);
    }
    else if (inBounds(pfr)) {
      fragColor = texture(from, pfr);
    }
    else {
      fragColor = bgColor(p, pfr, pto);
    }
  }
}

void main()
{
	mainImage(fFragColour, vTexCoord0*iResolution);
}
