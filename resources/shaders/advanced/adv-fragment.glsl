#version 400 core

in vec2 textCoords;
in vec3 surfaceNormal;
in vec3 toLightVector;
in vec3 toCameraVector;
in vec3 tangCoords;

out vec4 out_color;

uniform sampler2D textureSampler;
uniform sampler2D textureDarkSampler;
uniform sampler2D textureNormalSampler;
uniform sampler2D textureMaskSampler;

uniform vec3 lightColour;

uniform float shineDamper;
uniform float reflectivity;
uniform float diffuse;

void main(void){

	vec2 tex_coords = textCoords;
	tex_coords.y = 1.0 - tex_coords.y;

	vec4 normalMapValue = 2.0 * texture(textureNormalSampler, tex_coords) - 1.0;

	vec3 unitNormal = normalize(normalMapValue.xyz);
	vec3 unitLight = normalize(toLightVector);
	vec3 unitCamera = normalize(toCameraVector);
	vec3 lightDir = -unitLight;
	vec3 refLightDir = reflect(lightDir, unitNormal);

	float specularFactor = dot(refLightDir, unitCamera);
	specularFactor = max(specularFactor, 0.0);
	float dampedFactor = pow(specularFactor, shineDamper);
	vec3 finalSpec = dampedFactor * reflectivity * lightColour;

	float nDotl = dot(unitNormal, unitLight);
	float brightness = max(nDotl, diffuse);
	vec3 diffuse = brightness * lightColour;

	vec4 light_color = texture(textureSampler, tex_coords);
	vec4 dark_color = texture(textureDarkSampler, tex_coords);
	vec4 lighting = vec4(diffuse, 1.0);

	float s = smoothstep(-0.05, 0.05, nDotl);

	if (nDotl > 0.0) lighting = lighting + vec4(finalSpec, 1.0);

	out_color = lighting * (light_color * s + dark_color * (1.0 - s));
}