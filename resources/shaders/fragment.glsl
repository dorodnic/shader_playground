#version 400 core

in vec2 textCoords;

out vec4 out_color;

uniform sampler2D textureSampler;

void main(void){
	out_color = texture(textureSampler, textCoords);
}