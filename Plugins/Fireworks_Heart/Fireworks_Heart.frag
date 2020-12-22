uniform sampler2D		uTextureUnit0;
uniform float			uIntensity;
uniform vec2			iResolution;
uniform float			iTime;
uniform float			np;
uniform float			snp;

in vec2				vTexCoord0;
out vec4			fFragColour;

//fireworks in heart shape
//np = no. of particles
//snp = no. of spawned particles

#define rad(x) radians(x)
#define trail 1

//random value
vec2 N22(vec2 p){
    vec3 a = fract(p.xyx*vec3(123.34, 234.34, 345.65));
    a += dot(a, a+34.45);
	return fract(vec2(a.x*a.y, a.y*a.z));
}
float hash(vec2 uv){
 	return fract(sin(dot(uv,vec2(154.45,64.548))) * 124.54)  ; 
}

//particle
vec3 particle(vec2 st, vec2 p, float r, vec3 col){
 	float d = length(st-p);
    d = smoothstep(r, r-2.0/iResolution.y, d);//d<r?1.0:0.0;
    return d*col;
}
//particle with light
vec3 burst(vec2 st, vec2 pos, float r, vec3 col, int heart) {
	st -= pos;
    if (heart==1) st.y -= sqrt(abs(st.x))*0.1;
    r *=0.6*r;
    return (r/dot(st, st))*col*0.6;
}

//displacement with s = p0 + ut + 0.5at^2
vec2 get_pos(vec2 u, vec2 a, vec2 p0, float t, float ang){
    ang = rad(ang);
    vec2 d = p0 + vec2(u.x*cos(ang), u.y*sin(ang))*t + 0.5*a*t*t;
    return d;
}
//velocity at time t
vec2 get_velocity(vec2 u, vec2 a, float t, float ang){
    ang = rad(ang);
    return vec2(u.x*cos(ang), u.y*sin(ang)) + a*t;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from -1 to 1)
    vec2 uv = (2.*fragCoord-iResolution.xy)/iResolution.y;
    float aspect = iResolution.x/iResolution.y;
    vec3 col = vec3(0.0);
    float t = mod(iTime, 12.);
    
    //particle
    float r = 0.04;
    vec2 u = vec2(5.); //init velocity
    vec2 a = vec2(0.0, -9.8); //acc due to gravity
    float ang = 75.0; //angle of projection

    vec3 p1 = vec3(0.0); //particle
    
    for (float i=0.; i<np; i++)
    {
        vec2 rand = N22(vec2(i));
        vec2 ip = vec2(sin(15.*rand.x)*aspect, -1.+r); //initial pos
        u = vec2(sin(5.*rand.x), 5.+sin(4.*rand.y)); //initial velocity
        float t1 = t-i/5.;
    	vec2 s = get_pos(u, a, ip, t1, ang); // displacement
    	vec2 v = get_velocity(u, a, t1, ang);
        float Tf = 2.0*u.y*sin(rad(ang))/abs(a.y); //time of flight
		vec2 H = get_pos(u, a, ip, Tf/2.0, ang); //max height
        
        vec3 pcol = vec3(sin(22.*rand.x), sin(5.*rand.y), sin(1.*rand.x)); //color per particle

        if (v.y<-0.5){
            //p1+= burst(uv, H, max(0.0, 0.1-(t1*t1-Tf*0.5)), pcol*0.5, 1 );
        	r=0.0; //die
        }
        p1 += burst(uv, s, r, pcol, 0); //main particle
        
        //trail particle
        if (trail==1){
        	for (float k=4.0; k>0.0; k--){
                vec2 strail = get_pos(u, a, ip, t1-(k*0.02), ang); //delay for trail particles
        		p1 += burst(uv, strail, v.y<-0.5?0.0:r-(k*0.006), pcol, 0);
        	}
        }
        
        //spawn
        if (v.y<=0.0 && t1>=Tf/2.0)
        {
            for (float j=0.0; j<snp; j++)
            {
                vec2 rand2 = N22(vec2(j));
                float ang2 = (j*(360./snp));
                r = 0.035; // radius of spawned particles
                r -= (t1-Tf*0.5)*0.04;
                //r/=(t-(Tf/2.)+0.2);
                
                float x = cos(rad(ang2)); //coords of unit circle 
                float y = sin(rad(ang2));
                
                y = y + abs(x) * sqrt( (8.- abs(x))/50.0 ); // heart shape as a function of j
				
                vec2 heart = vec2(x*x + y*y)*(0.4/(t1*sqrt(t1))); //velocity vector
                vec2 S = get_pos(heart, a*0.03, H, t1-(Tf/2.), ang2);
                pcol = vec3(sin(8.*rand2.x), sin(6.*rand2.y), sin(2.*rand2.x)); // color per spawn particle
                p1 += burst(uv, S, max(0.0,r), pcol, 0);
                
            }
        } 

    }
    
    //bg
    float stars = pow(hash(uv),200.) * 0.5;
    col = p1;
    fragColor = vec4(col,1.0);
}

void main()
{
	fFragColour = texture(uTextureUnit0, vTexCoord0);
	vec4 fragColor;
	mainImage(fragColor, vec2(vTexCoord0.x, 1.0-vTexCoord0.y)*iResolution);
	fFragColour += fragColor;
}
