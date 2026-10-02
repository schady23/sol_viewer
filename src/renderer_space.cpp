#include "renderer.hpp"
namespace mars {
namespace {
const char* vertex=R"GLSL(#version 330 core
out vec2 screen;
void main() {
    screen=vec2((gl_VertexID<<1)&2,gl_VertexID&2)*2.-1.;
    gl_Position=vec4(screen,0,1);
})GLSL";
const char* fragment=R"GLSL(#version 330 core
in vec2 screen;
uniform vec3 rightward,upward,backward;
uniform float aspect,lens;
out vec4 color;
float hash(vec3 p) {
    p=fract(p*.1031); p+=dot(p,p.yzx+33.33);
    return fract((p.x+p.y)*p.z);
}
void main() {
    // Fixed directions at infinity: orbiting changes the sky; translating and
    // zooming do not drag a screen-space texture behind the planets.
    vec3 d=normalize(rightward*(screen.x*lens*aspect)+upward*(screen.y*lens)-backward);
    vec3 a=abs(d); vec2 uv; float face;
    if(a.x>=a.y&&a.x>=a.z) { uv=d.yz/a.x; face=d.x>0.?0.:1.; }
    else if(a.y>=a.z) { uv=d.xz/a.y; face=d.y>0.?2.:3.; }
    else { uv=d.xy/a.z; face=d.z>0.?4.:5.; }
    vec2 grid=(uv*.5+.5)*512.,cell=floor(grid);
    vec3 seed=vec3(cell,face*19.);
    float chance=hash(seed);
    vec2 centre=.22+.56*vec2(hash(seed+7.),hash(seed+41.));
    float radius=mix(.085,.24,hash(seed+13.));
    float aa=max(length(fwidth(grid))*.55,.025);
    float star=1.-smoothstep(max(0.,radius-aa),radius+aa,length(fract(grid)-centre));
    // Almost all cells remain empty. No bright nebula or twinkling noise.
    star*=step(.99915,chance)*mix(.16,.42,hash(seed+23.));
    vec3 tint=mix(vec3(.76,.84,1.),vec3(1.,.91,.78),hash(seed+31.));
    color=vec4(vec3(.003,.004,.006)+tint*star,1);
})GLSL";
}
void Renderer::spaceBackground(const Camera& camera) {
    gl_.BindFramebuffer(GL_FRAMEBUFFER,fbo_); gl_.Viewport(0,0,layout_.pixelWidth,layout_.pixelHeight);
    gl_.DepthMask(GL_TRUE); gl_.Clear(GL_DEPTH_BUFFER_BIT);
    gl_.Disable(GL_DEPTH_TEST); gl_.Disable(GL_CULL_FACE); gl_.Disable(GL_BLEND);
    if(!spaceProgram_) { spaceProgram_=program(vertex,fragment); gl_.GenVertexArrays(1,&spaceVao_); }
    gl_.UseProgram(spaceProgram_); gl_.BindVertexArray(spaceVao_);
    auto vec=[&](const char* name,Vec3 v) { gl_.Uniform3f(gl_.GetUniformLocation(spaceProgram_,name),v.x,v.y,v.z); };
    vec("rightward",camera.right()); vec("upward",camera.up()); vec("backward",camera.backward());
    gl_.Uniform1f(gl_.GetUniformLocation(spaceProgram_,"aspect"),float(layout_.pixelWidth)/layout_.pixelHeight);
    gl_.Uniform1f(gl_.GetUniformLocation(spaceProgram_,"lens"),camera.lens);
    gl_.DrawArrays(GL_TRIANGLES,0,3);
}
}
