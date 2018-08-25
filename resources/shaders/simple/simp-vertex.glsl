#version 400 core

in vec3 position;
in vec2 textureCoords;
in vec3 normal;
in vec3 tangent;

out vec3 surfaceNormal;
out vec3 toLightVector;
out vec3 toCameraVector;

out vec2 textCoords;

uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;
uniform mat4 cameraMatrix;

uniform vec3 lightPosition;

void main(void){
	vec4 worldPosition = transformationMatrix * vec4(position.xyz, 1.0);
	gl_Position = projectionMatrix * cameraMatrix * worldPosition;

	textCoords = textureCoords;

	surfaceNormal = (transformationMatrix * vec4(normal, 0.0)).xyz;
	toLightVector = lightPosition - worldPosition.xyz;
	toCameraVector = (inverse(cameraMatrix) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldPosition.xyz;
}