#version 400 core

in vec2 blurTexCoords[11];

out vec4 out_color;

uniform sampler2D textureSampler;
uniform float horizontal;

void main(void){
	out_color.w = 0;
	out_color = texture(textureSampler, blurTexCoords[5]) * (1 - horizontal) * vec4(1.0, 0.0, 0.0, 0.0);
	int best_i = -1;
	for (int i = 0; i < 11; i++)
	{
		vec4 tex_value = texture(textureSampler, blurTexCoords[i]);
		out_color.w += tex_value.z * (1.0 / 11.0);
		if (tex_value.z < 1 && abs(best_i - 5) > abs(i - 5))
			best_i = i;
	}

	out_color += texture(textureSampler, blurTexCoords[5]) * vec4(0.0, 0.0, 1.0, 0.0);
	out_color.w = 1 - out_color.w;

	if (best_i != 5)
		out_color += horizontal * vec4(1.0, 0.0, 0.0, 0.0) * (best_i / 11.0) +
					(1 - horizontal) * vec4(0.0, 1.0, 0.0, 0.0) * (best_i / 11.0);
}