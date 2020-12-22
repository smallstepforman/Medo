uniform sampler2D		uTextureUnit0;
uniform float			uIntensity;

in vec2				vTexCoord0;
out vec4			fFragColour;


void main()
{
	fFragColour = texture(uTextureUnit0, vTexCoord0);
	fFragColour *= pow(16.*vTexCoord0.x*vTexCoord0.y*(1.-vTexCoord0.x)*(1.-vTexCoord0.y), uIntensity); // Vigneting
}
