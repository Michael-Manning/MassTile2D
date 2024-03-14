#version 450

layout(set = 0, binding = 1) uniform CamerUBO {
    vec2 position;
    float zoom;
    float aspectRatio;
} camera;

#define MAX_PARTICLES_SMALL 400
#define MAX_PARTICLES_LARGE 400000

#define MAX_PARTICLE_SYSTEMS_SMALL 10
#define MAX_PARTICLE_SYSTEMS_LARGE 4

struct particle {
   vec2 position;
   vec2 velocity;
   float scale;
   float life;
   vec4 color;
};

struct ParticleGroup_small {
   particle particles[MAX_PARTICLES_SMALL];
};

struct ParticleGroup_large{
   particle particles[MAX_PARTICLES_LARGE];
};

layout(std430, set = 0, binding = 0) readonly buffer ObjectInstaceBuffer_small{
	ParticleGroup_small particleGroups_small[MAX_PARTICLE_SYSTEMS_SMALL];
};

layout(std430, set = 0, binding = 2) buffer ObjectInstaceBuffer_large{
	ParticleGroup_large particleGroups_large[MAX_PARTICLE_SYSTEMS_LARGE];
};

layout(push_constant) uniform constants{
   int systemIndex;
   int systemSize; // small = 0, large  = 1;
};

layout(location = 0) in vec2 inPosition;
layout(location = 2) in vec2 inFragCoord;

layout(location = 1) out vec2 uv;
layout(location = 2) out flat int instance_index;

mat4 translate(vec2 v) {
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        v.x, v.y, 0.0, 1.0
    );
}
mat4 scale(vec2 v) {
    return mat4(
        v.x, 0.0, 0.0, 0.0,
        0.0, v.y, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}
mat4 rotate(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat4(
        c,   s,   0.0, 0.0,
       -s,   c,   0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}


void main() {

    mat4 view = mat4(1.0);
    view *= scale(vec2(camera.zoom));
    view *= translate(camera.position);

   particle p = systemSize == 0 ? particleGroups_small[systemIndex].particles[gl_InstanceIndex] : particleGroups_large[systemIndex].particles[gl_InstanceIndex];

    mat4 model = mat4(1.0);
    model *= translate(p.position);
    model *= scale(vec2(p.scale) * step(0.0, p.life)); // step SHOULD hide dead particles

    gl_Position = view * model  * vec4(inPosition, 0.0, 1.0) * vec4(camera.aspectRatio, 1.0, 1.0, 1.0);

    instance_index = gl_InstanceIndex;
    uv = vec2(inFragCoord.x, 1.0 - inFragCoord.y);
}