#version 330 core
out vec4 FragColor;

in vec3 TexCoord;

uniform sampler3D texture1;

void main()
{
    vec4 color = texture(texture1, TexCoord);

    if (color.a <= 0.05) {
        discard;
    }

    FragColor = color;

   }