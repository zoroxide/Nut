#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;
layout(location=3) in float material;
layout(location=4) in vec3 tint;
uniform mat4 viewProj;
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 Tint;
flat out float Material;
void main() {
    FragPos=position; Normal=normal; TexCoords=uv; Tint=tint; Material=material;
    gl_Position=viewProj*vec4(position,1);
}
