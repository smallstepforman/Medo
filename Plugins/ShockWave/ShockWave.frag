//https://www.shadertoy.com/view/llj3Dz

uniform sampler2D		uTextureUnit0;
uniform float					iTime;
uniform vec2					uPos;
uniform float					uWaveX;
uniform float					uWaveY;
uniform float					uWaveZ;
uniform float					uFactorA;
uniform float					uFactorB;

in vec2				vTexCoord0;
out vec4			fFragColour;

void main()
{
 	vec3 WaveParams = vec3(uWaveX, uWaveY, uWaveZ ); 
 	
 	float t = iTime;
 	if (t < 0.1)
 		t = 0.05 + 0.05f*(t/0.1);	
    
    vec2 WaveCentre = vec2(uPos.x, uPos.y);
	float Dist = distance(vTexCoord0, WaveCentre);
	
	vec4 Color = texture(uTextureUnit0, vTexCoord0);
    
//Only distort the pixels within the parameter distance from the centre
if ((Dist <= ((iTime) + (WaveParams.z))) && 
	(Dist >= ((iTime) - (WaveParams.z)))) 
	{
        //The pixel offset distance based on the input parameters
		float Diff = (Dist - t); 
		float ScaleDiff = (1.0 - pow(abs(Diff * WaveParams.x), WaveParams.y)); 
		float DiffTime = (Diff  * ScaleDiff);
        
        //The direction of the distortion
		vec2 DiffTexCoord = normalize(vTexCoord0 - WaveCentre);         
        
        //Perform the distortion and reduce the effect over time
		vec2 distortion = ((DiffTexCoord * DiffTime) / (t * Dist * uFactorA));
		Color = texture(uTextureUnit0, vTexCoord0 + distortion);
        
        //Blow out the color and reduce the effect over time
		Color += (Color * ScaleDiff) / (t * Dist * uFactorB);
	} 
    
	fFragColour = Color; 
}
