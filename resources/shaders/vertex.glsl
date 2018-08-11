#version 400 core

in vec3 position;
in vec2 textureCoords;

out vec2 textCoords;

uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;
uniform mat4 cameraMatrix;

void main(void){
	gl_Position = projectionMatrix * cameraMatrix * transformationMatrix * vec4(position.xyz, 1.0);
	textCoords = textureCoords;
}