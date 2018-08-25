#version 400 core

in vec2 textCoords;

out vec4 out_color;

uniform sampler2D textureSampler;

void main(void){
	vec4 color = texture(textureSampler, textCoords);
	out_color = vec4(color.xyz, 1.0);
}