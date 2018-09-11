#version 400 core

in vec3 position;
in vec2 textureCoords;

uniform vec2 elementPosition;
uniform vec2 elementScale;

out vec2 blurTexCoords[11];

uniform int imageWidth;
uniform int imageHeight;
uniform float horizontal;

void main(void)
{
	gl_Position = vec4(position * vec3(elementScale, 1.0) + vec3(elementPosition, 0.0), 1.0);

	float pixelSize = 3.0 / float(imageWidth);
	for (int i = -5; i <= 5; i++)
	{
		blurTexCoords[i + 5] = textureCoords + 
			vec2(pixelSize, 0.0) * i * horizontal + 
			vec2(0.0, pixelSize) * i * (1.0 - horizontal);
	}
}