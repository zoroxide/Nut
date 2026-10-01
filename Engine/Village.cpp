#include "Village.h"
#include "Terrain.h"
#include "Textures.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

void Village::clear() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (albedo_) glDeleteTextures(1, &albedo_);
    if (normal_) glDeleteTextures(1, &normal_);
    if (shadowFbo_) glDeleteFramebuffers(1, &shadowFbo_);
    if (shadowTex_) glDeleteTextures(1, &shadowTex_);
    shadowFbo_ = shadowTex_ = 0;
    shadowBuilt_ = false;
    vao_ = vbo_ = albedo_ = normal_ = 0;
    count_ = 0; vertices_.clear(); walls_.clear();
}

void Village::quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int mat, glm::vec3 tint) {
    glm::vec3 n = glm::normalize(glm::cross(b-a, c-a));
    float u = glm::length(b-a) / 2, v = glm::length(d-a) / 2;
    Vertex q[] = {{a+origin_,n,{0,0},float(mat),tint}, {b+origin_,n,{u,0},float(mat),tint},
                  {c+origin_,n,{u,v},float(mat),tint}, {d+origin_,n,{0,v},float(mat),tint}};
    for (int i : {0,1,2}) vertices_.push_back(q[i]);
    // Gable triangles use c == d; do not emit a degenerate second triangle.
    if (glm::length(glm::cross(c-a, d-a)) > 1e-6f)
        for (int i : {0,2,3}) vertices_.push_back(q[i]);
}

void Village::box(glm::vec3 p, glm::vec3 s, int mat, bool solid, glm::vec3 tint) {
    glm::vec3 a=p-s*0.5f, b=p+s*0.5f;
    quad({a.x,a.y,b.z},{b.x,a.y,b.z},{b.x,b.y,b.z},{a.x,b.y,b.z},mat,tint);
    quad({b.x,a.y,a.z},{a.x,a.y,a.z},{a.x,b.y,a.z},{b.x,b.y,a.z},mat,tint);
    quad({a.x,a.y,a.z},{a.x,a.y,b.z},{a.x,b.y,b.z},{a.x,b.y,a.z},mat,tint);
    quad({b.x,a.y,b.z},{b.x,a.y,a.z},{b.x,b.y,a.z},{b.x,b.y,b.z},mat,tint);
    quad({a.x,b.y,b.z},{b.x,b.y,b.z},{b.x,b.y,a.z},{a.x,b.y,a.z},mat,tint);
    quad({a.x,a.y,a.z},{b.x,a.y,a.z},{b.x,a.y,b.z},{a.x,a.y,b.z},mat,tint);
    if (solid) walls_.push_back({a+origin_,b+origin_});
}

void Village::house(float x, float z, bool north, int variant) {
    // Front walls face the village lane. All doors are open, 1.8 m wide.
    float front = north ? z+4 : z-4, rear = north ? z-4 : z+4;
    glm::vec3 tint = variant%2 ? glm::vec3(0.92f,0.83f,0.69f) : glm::vec3(1,0.96f,0.86f);
    box({x,-0.3f,z},{10,0.6f,8},1,false);
    box({x,0.05f,z},{9.5f,0.10f,7.5f},4,false);
    box({x,1.7f,rear},{10,3.4f,0.3f},3);
    for (int side : {-1,1}) {
        // Side windows are real openings, with sills and timber frames.
        float sx=x+side*4.85f;
        box({sx,0.65f,z},{0.3f,1.3f,8},0,true,tint);
        box({sx,2.9f,z},{0.3f,1,8},0,true,tint);
        for (int end : {-1,1}) box({sx,1.85f,z+end*2.6f},{0.3f,1.1f,2.8f},0,true,tint);
        box({sx,1.3f,z},{0.55f,0.12f,2.5f},5,false);
        box({sx,1.85f,z},{0.14f,1.1f,0.09f},5,false);
        box({x+side*2.95f,1.7f,front},{4.1f,3.4f,0.3f},0,true,tint);
        box({x+side*0.98f,1.25f,front},{0.16f,2.5f,0.45f},5,false);
    }
    box({x,2.95f,front},{1.8f,0.9f,0.3f},0,true,tint);
    box({x,2.5f,front},{2.2f,0.18f,0.45f},5,false);
    for (float dx : {-4.85f,4.85f}) for(float dz : {-3.85f,3.85f})
        box({x+dx,1.7f,z+dz},{0.25f,3.5f,0.25f},5,false);
    box({x,3.4f,z},{10.5f,0.2f,8.6f},5,false);
    // Pitched roof; ceiling keeps the interior enclosed below the attic.
    quad({x-5.5f,3.5f,z+4.5f},{x+5.5f,3.5f,z+4.5f},{x+5.5f,5.5f,z},{x-5.5f,5.5f,z},2,{1,1,1});
    quad({x+5.5f,3.5f,z-4.5f},{x-5.5f,3.5f,z-4.5f},{x-5.5f,5.5f,z},{x+5.5f,5.5f,z},2,{1,1,1});
    for(int s : {-1,1}) quad({x+s*5,3.5f,z-4},{x+s*5,3.5f,z+4},{x+s*5,5.3f,z},{x+s*5,5.3f,z},0,tint);
    box({x+3.3f,4.5f,z-1},{0.85f,3,0.85f},1);
    // Bed, mattress, blanket and pillow.
    box({x-3.2f,0.3f,z+0.7f},{1.5f,0.6f,2.5f},5);
    box({x-3.2f,0.68f,z+0.7f},{1.45f,0.18f,2.45f},7,false,{0.72f,0.45f,0.3f});
    box({x-3.2f,0.84f,z-0.15f},{1.05f,0.16f,0.5f},7,false);
    // Dining table and stools, cupboard and shelves.
    box({x+2.4f,0.88f,z},{2,0.16f,1.3f},5);
    for(float dx : {-0.8f,0.8f}) for(float dz : {-0.45f,0.45f})
        box({x+2.4f+dx,0.4f,z+dz},{0.12f,0.8f,0.12f},5);
    for(int s : {-1,1}) box({x+2.4f,0.3f,z+s*1.2f},{0.6f,0.6f,0.6f},5);
    box({x+2.8f,1.1f,rear+(north?0.55f:-0.55f)},{2,2.2f,0.7f},6);
    box({x,1.65f,rear+(north?0.4f:-0.4f)},{2.6f,0.12f,0.5f},5,false);
    for(int i=0;i<3;++i) box({x-0.8f+i*0.7f,1.88f,rear+(north?0.4f:-0.4f)},{0.3f,0.35f,0.3f},1,false);
    // A hearth at the back, with a dark firebox and stone mantel.
    box({x-2.8f,0.3f,rear+(north?0.6f:-0.6f)},{1.6f,0.6f,0.9f},1);
    box({x-2.8f,1.15f,rear+(north?0.3f:-0.3f)},{1.2f,1.2f,0.25f},5,false,{0.15f,0.15f,0.15f});
    box({x-2.8f,1.8f,rear+(north?0.6f:-0.6f)},{1.8f,0.2f,1},1);
    // Individual front path and bench.
    float pathEnd = north ? -8.5f : 8.5f;
    box({x,0.04f,(front+pathEnd)*0.5f},{2.3f,0.08f,std::abs(front-pathEnd)},1,false);
    box({x+3,0.5f,front+(north?1.1f:-1.1f)},{2,0.18f,0.65f},5);
}

void Village::generate(Terrain& terrain) {
    clear();
    if (terrain.isFlat() || terrain.getHalfExtent()<85) return;
    // Find a dry, relatively level clearing with space for the complete settlement.
    float best=1e30f, half=terrain.getHalfExtent()-65;
    for(float z=-half;z<=half;z+=24) for(float x=-half;x<=half;x+=24) {
        float low=1e30f, high=-1e30f, sum=0;
        for(int j=-1;j<=1;++j) for(int i=-1;i<=1;++i) {
            float h=terrain.getHeightAt(x+i*45.0f,z+j*35.0f);
            low=std::min(low,h); high=std::max(high,h); sum+=h;
        }
        if(low<terrain.getWaterY()+2) continue;
        float score=high-low+0.008f*std::sqrt(x*x+z*z);
        if(score<best) { best=score; origin_={x,sum/9,z}; }
    }
    if(best==1e30f) { std::cerr<<"Village: no dry site large enough on this terrain\n"; return; }
    terrain.editHeights({origin_.x-62,origin_.z-62},{origin_.x+62,origin_.z+62},[&](float x,float z,float h){
        float d=std::max(std::abs(x-origin_.x),std::abs(z-origin_.z));
        return glm::mix(origin_.y,h,glm::smoothstep(47.0f,62.0f,d));
    });
    const char* names[]={"plaster","stone","roof","interior","floor","wood","painted","fabric"};
    albedo_=Textures::loadMaterialArray("assets/textures/village",names,8,"_albedo",false,8);
    normal_=Textures::loadMaterialArray("assets/textures/village",names,8,"_normal",true,8);
    // Paving slabs meet at their edges; no coplanar surfaces at the crossroads.
    box({0,0.04f,0},{78,0.08f,17},1,false,{0.8f,0.78f,0.7f});
    box({0,0.04f,-14.25f},{7,0.08f,11.5f},1,false);
    box({0,0.04f,27.25f},{7,0.08f,37.5f},1,false);
    for(int i=0;i<4;++i) { house(-30+i*20,-17,true,i); house(-30+i*20,17,false,i+1); }
    // Square well, open at the top, with timber posts and a canopy.
    for(int s : {-1,1}) {
        box({s*1.3f,0.6f,0},{0.4f,1.2f,3},1);
        box({0,0.6f,s*1.3f},{2.6f,1.2f,0.4f},1);
        box({s*1.8f,1.6f,0},{0.2f,3.2f,0.2f},5);
    }
    box({0,0.08f,0},{2.2f,0.08f,2.2f},6,false,{0.2f,0.35f,0.4f});
    box({0,3.2f,0},{4.5f,0.2f,3.7f},2,false);
    box({0,2.5f,0},{3.6f,0.15f,0.15f},5,false);
    // Market stalls with striped fabric awnings, produce crates and stock.
    for(int s : {-1,1}) {
        float x=s*12.0f;
        box({x,0.9f,3},{4,0.2f,1.8f},5);
        for(int a : {-1,1}) for(int b : {-1,1}) box({x+a*1.8f,1.35f,3+b*0.8f},{0.12f,2.7f,0.12f},5);
        for(int i=0;i<6;++i) box({x-1.75f+i*0.7f,2.7f,3},{0.7f,0.08f,2.5f},7,false,i%2?glm::vec3(0.85f,0.3f,0.18f):glm::vec3(1,0.9f,0.65f));
        for(int i=0;i<3;++i) {
            box({x-1.2f+i*1.2f,1.12f,3},{0.95f,0.25f,1.1f},5);
            box({x-1.2f+i*1.2f,1.3f,3},{0.75f,0.15f,0.8f},7,false,glm::vec3(0.7f,0.25f+i*0.22f,0.12f));
        }
        for(int i=0;i<3;++i) box({x+i*0.9f,0.45f,6},{0.8f,0.9f,0.8f},5);
    }
    // Garden fences behind houses, with gaps along the central access path.
    for(int s : {-1,1}) for(int x=-42;x<=42;x+=3) {
        if(std::abs(x)<6) continue;
        box({float(x),0.65f,s*29.0f},{0.16f,1.3f,0.16f},5);
        box({x+1.4f,0.85f,s*29.0f},{2.8f,0.13f,0.13f},5);
    }
    glGenVertexArrays(1,&vao_); glGenBuffers(1,&vbo_);
    glBindVertexArray(vao_); glBindBuffer(GL_ARRAY_BUFFER,vbo_);
    glBufferData(GL_ARRAY_BUFFER,vertices_.size()*sizeof(Vertex),vertices_.data(),GL_STATIC_DRAW);
    const int sizes[]={3,3,2,1,3};
    const size_t offsets[]={offsetof(Vertex,p),offsetof(Vertex,n),offsetof(Vertex,uv),offsetof(Vertex,material),offsetof(Vertex,tint)};
    for(int i=0;i<5;++i) { glEnableVertexAttribArray(i); glVertexAttribPointer(i,sizes[i],GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsets[i]); }
    glBindVertexArray(0); count_=GLsizei(vertices_.size()); vertices_.clear();
    std::cout<<"Village: 8 furnished cottages at "<<origin_.x<<", "<<origin_.z<<"\n";
}

void Village::draw(GLuint p,const glm::mat4& view,const glm::mat4& proj) const {
    if(!active() || !p) return;
    glUseProgram(p); glm::mat4 vp=proj*view;
    glUniformMatrix4fv(glGetUniformLocation(p,"viewProj"),1,GL_FALSE,&vp[0][0]);
    glUniform1i(glGetUniformLocation(p,"albedoMaps"),0);
    glUniform1i(glGetUniformLocation(p,"normalMaps"),1);
    glUniform1i(glGetUniformLocation(p,"hasAlbedo"),albedo_!=0);
    glUniform1i(glGetUniformLocation(p,"hasNormal"),normal_!=0);
    glUniform3fv(glGetUniformLocation(p,"villageCenter"),1,&origin_.x);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D_ARRAY,albedo_);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D_ARRAY,normal_);
    GLboolean cull=glIsEnabled(GL_CULL_FACE); glDisable(GL_CULL_FACE);
    glBindVertexArray(vao_); glDrawArrays(GL_TRIANGLES,0,count_); glBindVertexArray(0);
    if(cull) glEnable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
}

void Village::buildShadow(GLuint program, const glm::vec3& sunDir) {
    if (!active() || !program) return;
    glm::vec3 sun = glm::normalize(sunDir);
    if (sun.y <= 0.0f) {
        shadowBuilt_ = false;
        return;
    }
    if (shadowBuilt_ && glm::dot(sun, shadowSun_) > 0.999999f) return;

    GLint prevDrawFbo, prevReadFbo, viewport[4], prevProgram, prevVAO, depthFunc;
    GLboolean depthMask;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFbo);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    GLboolean depth = glIsEnabled(GL_DEPTH_TEST), cull = glIsEnabled(GL_CULL_FACE);
    GLboolean blend = glIsEnabled(GL_BLEND);

    if (!shadowTex_) {
        glGenTextures(1, &shadowTex_);
        glBindTexture(GL_TEXTURE_2D, shadowTex_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, kShadowResolution,
                     kShadowResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        const float border[] = {1, 1, 1, 1};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
        glGenFramebuffers(1, &shadowFbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex_, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Village: could not create sun shadow framebuffer\n";
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFbo);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFbo);
            glDeleteFramebuffers(1, &shadowFbo_);
            glDeleteTextures(1, &shadowTex_);
            shadowFbo_ = shadowTex_ = 0;
            return;
        }
    }

    // A fixed village-relative light volume avoids camera-induced shadow shimmer.
    // Use a different up vector near midday, where sun and world-up are parallel.
    glm::vec3 up = std::abs(sun.y) > 0.98f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
    glm::vec3 target = origin_ + glm::vec3(0, 2, 0);
    shadowMatrix_ = glm::ortho(-76.0f, 76.0f, -76.0f, 76.0f, 1.0f, 240.0f) *
                    glm::lookAt(target + sun * 120.0f, target, up);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
    glViewport(0, 0, kShadowResolution, kShadowResolution);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "lightViewProj"), 1, GL_FALSE, &shadowMatrix_[0][0]);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, count_);

    glBindVertexArray(prevVAO);
    glUseProgram(prevProgram);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFbo);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDepthFunc(depthFunc);
    glDepthMask(depthMask);
    if (!depth) glDisable(GL_DEPTH_TEST);
    if (cull) glEnable(GL_CULL_FACE);
    if (blend) glEnable(GL_BLEND);
    shadowSun_ = sun;
    shadowBuilt_ = true;
}

void Village::bindShadow(GLuint program, int unit, float strength, bool enabled) const {
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "villageShadowTex"), unit);
    glUniform1i(glGetUniformLocation(program, "hasVillageShadow"), enabled && shadowBuilt_ ? 1 : 0);
    glUniform1f(glGetUniformLocation(program, "villageShadowStrength"), strength);
    glUniformMatrix4fv(glGetUniformLocation(program, "villageLightViewProj"), 1, GL_FALSE, &shadowMatrix_[0][0]);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, shadowTex_);
    glActiveTexture(GL_TEXTURE0);
}

void Village::collide(glm::vec3& eye) const {
    constexpr float r=0.32f;
    for(const auto& w:walls_) {
        if(eye.y-1.65f>=w.hi.y || eye.y<=w.lo.y) continue;
        float lx=w.lo.x-r,hx=w.hi.x+r,lz=w.lo.z-r,hz=w.hi.z+r;
        if(eye.x<=lx || eye.x>=hx || eye.z<=lz || eye.z>=hz) continue;
        float d[]={eye.x-lx,hx-eye.x,eye.z-lz,hz-eye.z};
        int k=int(std::min_element(d,d+4)-d);
        if(k==0) eye.x=lx;
        if(k==1) eye.x=hx;
        if(k==2) eye.z=lz;
        if(k==3) eye.z=hz;
    }
}
