#version 400 core

in vec2 textCoords;

out vec4 out_color;

uniform sampler2D textureSampler;

uniform int decalsCount;

void main(void){
	vec4 color = texture(textureSampler, textCoords);

	vec2 center = vec2(0.5, 0.5);
	float l = length(fract(textCoords * decalsCount) - center);

	l = l * 4.0;

	l = 1 - max(0, min(1, l));

	out_color = vec4(color.xyz * l, 1.0);
}