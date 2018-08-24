#version 400 core

in vec2 textCoords;
in vec3 surfaceNormal;
in vec3 toLightVector;
in vec3 toCameraVector;
in vec4 clipSpace;

out vec4 out_color;

uniform sampler2D textureSampler;
uniform sampler2D refractionSampler;

uniform float shineDamper;
uniform float reflectivity;
uniform float ambient;

void main(void){
	vec2 tex = vec2(textCoords.x, 1 - textCoords.y);

	vec3 unitNormal = normalize(surfaceNormal);
	vec3 lightDir = normalize(toLightVector);
	vec3 unitCamera = normalize(toCameraVector);
	vec3 refLightDir = reflect(-lightDir, unitNormal);

	float specularFactor = dot(refLightDir, unitCamera);
	specularFactor = max(specularFactor, 0.0);
	float dampedFactor = pow(specularFactor, shineDamper);
	vec4 finalSpec = dampedFactor * reflectivity * vec4(1.0, 1.0, 1.0, 1.0);

	float nDotl = dot(unitNormal, lightDir);
	float nDotc = dot(unitNormal, unitCamera);
	nDotc = abs(nDotc);
	float brightness = max(nDotl, ambient);
	vec4 lighting = brightness * vec4(1.0, 1.0, 1.0, 1.0);

	vec4 color = texture(textureSampler, tex);

	vec2 ndc = ((clipSpace.xy / clipSpace.w) / 2.0 + 0.5);
	vec4 color2 = texture(refractionSampler, ndc);

	float s = smoothstep(-0.05, 0.05, nDotl);

	lighting = lighting + finalSpec;

	out_color = lighting * mix(color, color2, nDotc);
}