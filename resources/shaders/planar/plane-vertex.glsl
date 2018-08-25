#version 400 core

in vec3 position;
in vec2 textureCoords;

out vec2 textCoords;

uniform vec2 elementPosition;
uniform vec2 elementScale;

void main(void)
{
	gl_Position = vec4(position * vec3(elementScale, 1.0) + vec3(elementPosition, 0.0), 1.0);
	textCoords = textureCoords;
}