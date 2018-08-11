#version 400 core

in vec2 textCoords;
in vec3 surfaceNormal;
in vec3 toLightVector;
in vec3 toCameraVector;

out vec4 out_color;

uniform sampler2D textureSampler;

uniform vec3 lightColour;

uniform float shineDamper;
uniform float reflectivity;

void main(void){

	vec3 unitNormal = normalize(surfaceNormal);
	vec3 unitLight = normalize(toLightVector);
	vec3 unitCamera = normalize(toCameraVector);
	vec3 lightDir = -unitLight;
	vec3 refLightDir = reflect(lightDir, unitNormal);

	float specularFactor = dot(refLightDir, unitCamera);
	specularFactor = max(specularFactor, 0.0);
	float dampedFactor = pow(specularFactor, shineDamper);
	vec3 finalSpec = dampedFactor * reflectivity * lightColour;

	float nDotl = dot(unitNormal, unitLight);
	float brightness = max(nDotl, 0.2);
	vec3 diffuse = brightness * lightColour;

	vec2 tex_coords = textCoords;
	tex_coords.y = 1.0 - tex_coords.y;

	out_color = vec4(finalSpec, 1.0) + vec4(diffuse, 1.0) * texture(textureSampler, tex_coords);
}