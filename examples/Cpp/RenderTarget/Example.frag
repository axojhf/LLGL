// GLSL model fragment shader

#version 140

layout(std140) uniform Settings
{
	mat4 wvpMatrix;
	mat4 wMatrix;
	int useTexture2DMS;
};

uniform sampler2D colorMap;
//uniform sampler2DMS colorMapMS;

in vec3 vNormal;
in vec2 vTexCoord;

out vec4 fragColor;

vec4 SampleColorMap(vec2 texCoord)
{
    #if 0
	if (useTexture2DMS != 0)
	{
		// Fetch texel from multi-sample texture
		ivec2 size = textureSize(colorMapMS);
		int numSamples = textureSamples(colorMapMS);
		
		ivec2 tc = ivec2(
			int(texCoord.x * float(size.x)),
			int(texCoord.y * float(size.y))
		);
		
		// Compute average of all samples
		vec4 c = vec4(0);
		
		for (int i = 0; i < numSamples; ++i)
			c += texelFetch(colorMapMS, tc, i);
		
		c /= numSamples;
		
		return c;
	}
	else
    #endif
	{
		// Sample texel from standard texture
		return texture(colorMap, texCoord);
	}
}

void main()
{
    vec4 color = SampleColorMap(vTexCoord);
    
	// Apply lambert factor for simple shading
	const vec3 lightVec = vec3(0, 0, -1);
	float NdotL = dot(lightVec, normalize(vNormal));
	color.rgb *= mix(0.2, 1.0, NdotL);
    
    fragColor = color;
}
