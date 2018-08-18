#version 400 core

out vec4 out_color;
in vec3 out_arrowColour;

void main(void){
	out_color = vec4(out_arrowColour, 1);
}