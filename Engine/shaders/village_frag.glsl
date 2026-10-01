#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 Tint;
flat in float Material;
out vec4 FragColor;
uniform sampler2DArray albedoMaps;
uniform sampler2DArray normalMaps;
uniform bool hasAlbedo;
uniform bool hasNormal;
#include "common.glsl"
void main() {
    vec3 n=normalize(Normal)*(gl_FrontFacing?1.0:-1.0);
    vec3 dp1=dFdx(FragPos), dp2=dFdy(FragPos);
    vec2 du1=dFdx(TexCoords), du2=dFdy(TexCoords);
    vec3 t=cross(dp2,n)*du1.x+cross(n,dp1)*du2.x;
    vec3 b=cross(dp2,n)*du1.y+cross(n,dp1)*du2.y;
    float scale=inversesqrt(max(max(dot(t,t),dot(b,b)),0.00001));
    if(hasNormal) {
        vec2 xy=texture(normalMaps,vec3(TexCoords,Material)).rg*2.0-1.0;
        n=normalize(mat3(t*scale,b*scale,n)*vec3(xy,sqrt(max(0.01,1.0-dot(xy,xy)))));
    }
    vec3 base=Tint*(hasAlbedo?texture(albedoMaps,vec3(TexCoords,Material)).rgb:vec3(0.7));
    float diffuse=max(dot(n,normalize(-lightDir)),0.0);
    vec3 color=base*(max(skyAmbient(n)*0.55,vec3(0.16))+diffuse*lightColor*sunShadow(FragPos+n*0.2));
    FragColor=vec4(applyFog(color,FragPos),1);
}
