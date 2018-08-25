#version 400 core

in vec3 position;
in vec2 textureCoords;
in vec3 normal;
in vec3 tangent;

out vec3 surfaceNormal;
out vec3 toLightVector;
out vec3 toCameraVector;

out vec2 textCoords;
out vec4 clipSpace;

uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;
uniform mat4 cameraMatrix;

uniform vec3 lightPosition;

void main(void){
	vec4 worldPosition = transformationMatrix * vec4(position.xyz, 1.0);

	textCoords = textureCoords;

	surfaceNormal = (transformationMatrix * vec4(normal, 0.0)).xyz;

	vec3 norm = normalize(surfaceNormal);
	vec3 tang = normalize((transformationMatrix * vec4(tangent, 0.0)).xyz);
	vec3 bitang = normalize(cross(norm, tang));

	mat3 toTangentSpace = mat3(
		tang.x, bitang.x, norm.x,
		tang.y, bitang.y, norm.y,
		tang.z, bitang.z, norm.z
	);

	clipSpace = projectionMatrix * cameraMatrix * worldPosition;
	gl_Position = clipSpace;

	surfaceNormal = toTangentSpace * surfaceNormal;
	toLightVector = toTangentSpace * (lightPosition - worldPosition.xyz);
	toCameraVector = toTangentSpace * ((inverse(cameraMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldPosition.xyz);
}