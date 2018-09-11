#version 400 core

in vec2 textCoords;
in vec3 surfaceNormal;
in vec3 toLightVector;
in vec3 toCameraVector;

out vec4 out_color;

uniform sampler2D textureSampler;

void main(void){
	out_color = vec4(surfaceNormal.xy, 0.0, 1.0);
	//out_color = vec4(1.0, 1.0, 1.0, 1.0);
}