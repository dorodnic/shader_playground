#version 400 core

in vec3 position;
in vec2 textureCoords;

out vec2 textCoords;

uniform mat4 transformationMatrix;

void main(void){
	gl_Position = transformationMatrix * vec4(position.xyz, 1.0);
	textCoords = textureCoords;
}