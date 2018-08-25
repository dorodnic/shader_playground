#version 400 core

in vec2 textCoords;
in vec3 surfaceNormal;
in vec3 toLightVector;
in vec3 toCameraVector;
in vec4 clipSpace;

out vec4 out_color;

uniform sampler2D textureSampler;
uniform sampler2D refractionSampler;
uniform sampler2D textureNormalSampler;

uniform float shineDamper;
uniform float reflectivity;
uniform float shineDamper2;
uniform float reflectivity2;
uniform float ambient;
uniform float distortion;

void main(void){
	vec2 tex = vec2(textCoords.x, 1 - textCoords.y);

	vec4 normalMapValue = 2.0 * texture(textureNormalSampler, tex) - 1.0;

	vec3 unitNormal = normalize(surfaceNormal.xyz);

	vec3 lightDir = normalize(toLightVector);
	vec3 unitCamera = normalize(toCameraVector);

	float nDotc0 = dot(unitNormal, unitCamera);
	if (nDotc0 > 0) {
		unitNormal = normalize(normalMapValue.xyz);
	}

	vec3 refLightDir = reflect(-lightDir, unitNormal);

	float specularFactor = dot(refLightDir, unitCamera);
	specularFactor = max(specularFactor, 0.0);
	float dampedFactor = pow(specularFactor, shineDamper);
	vec4 finalSpec = dampedFactor * reflectivity * vec4(1.0, 1.0, 1.0, 1.0);

	float dampedFactor2 = pow(specularFactor, shineDamper2);
	vec4 finalSpec2 = dampedFactor2 * reflectivity2 * vec4(1.0, 1.0, 1.0, 1.0);

	float nDotl = dot(unitNormal, lightDir);
	float nDotc = abs(nDotc0);
	float brightness = max(nDotl, ambient);
	vec4 lighting = brightness * vec4(1.0, 1.0, 1.0, 1.0);

	vec4 color = texture(textureSampler, tex);

	float mask_val = 1 - color.w;
	color.w = 1.0;

	vec2 clip_xy = clipSpace.xy;

	nDotc0 = max(min(abs(nDotc0), 1), 0);
	nDotc0 = pow(nDotc0, 5);
	clip_xy = mix(1 - distortion, 1 + distortion, nDotc0) * clip_xy;

	vec2 ndc = ((clip_xy / clipSpace.w) / 2.0 + 0.5);

	vec4 color2 = texture(refractionSampler, ndc);

	float s = smoothstep(-0.05, 0.05, nDotl);

	float refract_factor = pow(nDotc, 2);

	vec4 lighting2 = lighting + finalSpec2;
	vec4 lighting1 = lighting + finalSpec * (1 - refract_factor);
	

	vec4 glassTotal = mix(color, color2, refract_factor);
	out_color = mix(lighting2 *color, lighting1 * glassTotal, mask_val);

	//out_color.x = tex.x;
}