#version 400 core

in vec2 textCoords;
in vec3 surfaceNormal;
in vec3 toLightVector;
in vec3 toCameraVector;
in vec4 clipSpace;
in vec3 refractedVector;

out vec4 out_color;

uniform sampler2D textureSampler;
uniform sampler2D refractionSampler;
uniform sampler2D textureNormalSampler;
uniform sampler2D destructionSampler;

uniform float shineDamper;
uniform float reflectivity;
uniform float shineDamper2;
uniform float reflectivity2;
uniform float ambient;
uniform float distortion;

uniform float do_normal_mapping;

uniform vec2 decal_uvs;
uniform int decal_id;
uniform int decal_variations;

precision lowp    float;

float PHI = 1.61803398874989484820459 * 00000.1; // Golden Ratio   
float PI  = 3.14159265358979323846264 * 00000.1; // PI
float SQ2 = 1.41421356237309504880169 * 10000.0; // Square Root of Two

float gold_noise(in vec2 coordinate, in float seed){
    return fract(tan(distance(coordinate*(seed+PHI), vec2(PHI, PI)))*SQ2);
}

void main(void){
	float reflectivity_copy = reflectivity;
	float ambient_copy = ambient;
	float distortion_copy = distortion;
	float refraction_killer = 0.0;

	vec2 tex = vec2(textCoords.x, 1 - textCoords.y);

	vec4 normalMapValue = 2.0 * texture(textureNormalSampler, tex) - 1.0;

	vec3 unitNormal = normalize(surfaceNormal.xyz);

	vec3 lightDir = normalize(toLightVector);
	vec3 unitCamera = normalize(toCameraVector);

	float nDotc0 = dot(unitNormal, unitCamera);
	if (nDotc0 * do_normal_mapping > 0) {
		unitNormal = normalize(normalMapValue.xyz);
	}

	vec4 color = texture(textureSampler, tex);

	float mask_val = 1 - color.w;
	color.w = 1.0;

	vec2 clip_xy = clipSpace.xy;

	vec2 clip_xyn = ((clip_xy / clipSpace.w) / 2.0 + 0.5);

	vec2 decal_tex = vec2(
		((decal_uvs.x - tex.x)* 2.0 + 0.5) , 
		((1.0 - decal_uvs.y - tex.y)* 1.2 + 0.5)
	);
	decal_tex.x = max(0, min(1, decal_tex.x));
	decal_tex.y = max(0, min(1, decal_tex.y));

	int i = decal_id / decal_variations;
	int j = decal_id % decal_variations;
	decal_tex.x = decal_tex.x / decal_variations + float(i) / decal_variations;
	decal_tex.y = decal_tex.y / decal_variations + float(j) / decal_variations;

	decal_tex.x = max(0, min(1, decal_tex.x));
	decal_tex.y = max(0, min(1, decal_tex.y));

	vec4 dest_space = texture(destructionSampler, decal_tex);
	//out_color = vec4(decal_tex, 1.0, 1.0);

	float near_decal = (1 - dest_space.w);

	if ((dest_space.z == 1) && do_normal_mapping > 0 && mask_val > 0 && nDotc0 > 0)
	{
		vec2 p = unitCamera.xy / unitCamera.z;
		float dx = (dest_space.x - 0.5) * 2;
		float dy = (dest_space.y - 0.5) * 2;
		//
		if (((dx * p.x < 0) && (abs(dx) < 0.9) &&
			(abs(dx) < abs(p.x)))
			||
			((dy * p.y < 0) && (abs(dy) < 0.9) &&
			(abs(dy) < abs(p.y)))
			)
		{
			float nx = gold_noise(p, 5.0);
			float ny = gold_noise(p, 7.0);

			unitNormal = unitCamera * vec3(
				dx * 0.2 + 0.9, 
				dy * 0.2 + 0.9, 0.0);

			unitNormal = 0.5 * unitNormal + vec3(-dx,-dy,0.0) * 0.5;

			float factor = (nx * 0.2) + 0.8;
			color.x *= factor;
			color.y *= factor;
			color.z *= factor;

			unitNormal = normalize(unitNormal);
			distortion_copy = 0.0;
			reflectivity_copy = reflectivity_copy * 0.5;
			ambient_copy = ambient_copy * 0.5;
			nDotc0 = dot(unitNormal, unitCamera);
			refraction_killer = 1.0;
			near_decal = 0.0;
		}
		else
		{
			out_color = texture(refractionSampler, clip_xyn) * 0.5;
			return;
		}
	}

	//vec3 to_decal = normalize(vec3(decal_uvs.x - tex.x, decal_uvs.y - tex.y, 0.0));
	//unitNormal = unitNormal + near_decal * vec3(to_decal);
	decal_tex = vec2(
		((decal_uvs.x - tex.x)* 5.0 ) , 
		((1.0 - decal_uvs.y - tex.y)* 5.0)
	);
	unitNormal = unitNormal + sin(near_decal) * vec3(decal_tex, 0.0);
	unitNormal = normalize(unitNormal);

	vec3 refLightDir = reflect(-lightDir, unitNormal);

	float specularFactor = dot(refLightDir, unitCamera);
	specularFactor = max(specularFactor, 0.0);
	float dampedFactor = pow(specularFactor, shineDamper);
	vec4 finalSpec = dampedFactor * reflectivity_copy * vec4(1.0, 1.0, 1.0, 1.0);

	float dampedFactor2 = pow(specularFactor, shineDamper2);
	vec4 finalSpec2 = dampedFactor2 * reflectivity2 * vec4(1.0, 1.0, 1.0, 1.0);

	float nDotl = dot(unitNormal, lightDir);
	float nDotc = abs(nDotc0);
	float brightness = max(nDotl, ambient_copy);
	vec4 lighting = brightness * vec4(1.0, 1.0, 1.0, 1.0);

	nDotc0 = max(min(abs(nDotc0), 1), 0);
	nDotc0 = pow(nDotc0, 5);
	clip_xy = mix(1 - distortion_copy, 1 + distortion_copy, nDotc0) * clip_xy;

	vec2 ndc = ((clip_xy / clipSpace.w) / 2.0 + 0.5);

	vec4 color2 = texture(refractionSampler, ndc);

	float refract_factor = pow(nDotc, 2) * (1.0 - refraction_killer);

	vec4 lighting2 = lighting + finalSpec2;
	vec4 lighting1 = lighting + finalSpec * (1 - refract_factor);
	

	vec4 glassTotal = mix(color, color2, refract_factor);

	out_color = mix(lighting2 *color, lighting1 * glassTotal, mask_val);

	//out_color.x = tex.x;
}