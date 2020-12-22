uniform sampler2D		iChannel0;
uniform sampler2D		iChannel1;
uniform vec2			iResolution;
uniform float			iTime;
uniform float			iFactorY;
uniform float			radius;

in vec2				vTexCoord0;
out vec4			fFragColour;

// Created by Eduardo Castineyra - casty/2015
// Creative Commons Attribution 4.0 International License

#define PI 3.141592
//const float radius = 0.3;
#define DIST 2
vec3 cyl = vec3(0.0);

/// 1D function x: cylFun(t); y: normal at that point.
vec2 curlFun(float t, float maxt){
	vec2 ret = vec2(t, 1.0);    
    if (t < cyl[DIST] - radius)
        return ret;					/// Before the curl
	if (t > cyl[DIST] + radius)
        return vec2(-1.0);			/// After the curl
    
    /// Inside the curl
    float a = asin((t - cyl[DIST]) / radius);
    float ca = -a + PI;
    ret.x = cyl[DIST] + ca * radius;
    ret.y = cos(ca);
    
    if (ret.x < maxt)  
        return ret;					/// We see the back face

    if (t < cyl[DIST])
        return vec2(t, 1.0);		/// Front face before the curve starts
    ret.y = cos(a);
    ret.x = cyl[DIST] + a * radius;
    return ret.x < maxt ? ret : vec2(-1.0);  /// Front face curve
	}

void main()
{
    vec2 uv = vec2(vTexCoord0.x, vTexCoord0.y);
    //vec2 ur = vec2(1.0, iResolution.y/iResolution.x);
	vec2 ur = vec2(1.0, 1.0);
    vec2 mouse = vec2(1-iTime, (1-iTime)/iFactorY);
    float d = length(mouse * (1.0 + 4.0*radius)) - 2.0*radius;
    cyl = vec3(normalize(mouse), d);
    
    d = dot(uv, cyl.xy);
    vec2 end = abs((ur - uv) / cyl.xy);
    float maxt = d + min(end.x, end.y);
    vec2 cf = curlFun(d, maxt);
    vec2 tuv = uv + cyl.xy * (cf.x - d);
    
	float shadow = 1.0 - smoothstep (0.0, radius * 2.0, -(d - cyl[DIST]));
   	shadow *= (smoothstep(-radius, radius, (maxt - (cf.x + 1.5 * PI * radius + radius))));
    vec4 curr = texture(iChannel1, tuv / ur, -100.0);
    curr = cf.y > 0.0 ? curr * cf.y  * (1.0 - shadow): (curr * 0.25 + 0.75) * (-cf.y);
    shadow = smoothstep (0.0, radius * 2.0, (d - cyl[DIST]));
    vec4 next = texture(iChannel0, uv, -100.0) * shadow;
    fFragColour = cf.x > 0.0 ? curr : next;
}

