#version 400 core

in vec3 position;
in vec2 textureCoords;

out vec2 textCoords;

void main(void){
	gl_Position = vec4(position.xyz, 1.0);
	textCoords = textureCoords;
}