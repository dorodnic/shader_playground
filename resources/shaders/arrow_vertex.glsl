#version 400 core

in vec3 position;

uniform vec3 arrowColour;

out vec3 out_arrowColour;

uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;
uniform mat4 cameraMatrix;

void main(void){
	vec4 worldPosition = transformationMatrix * vec4(position.xyz, 1.0);
	gl_Position = projectionMatrix * cameraMatrix * worldPosition;
	out_arrowColour = arrowColour;
}