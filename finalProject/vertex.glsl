#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec3 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float profundidad;
uniform mat4 Ttex;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0f);

    vec2 flippedTexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);

    vec4 textemp = Ttex * vec4(flippedTexCoord.x, flippedTexCoord.y, profundidad, 1.0);
    TexCoord = vec3(textemp.x, textemp.y, textemp.z);
}