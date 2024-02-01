#version 450

layout(set = 0, binding = 1) uniform CamerUBO {
    vec2 position;
    float zoom;
    float aspectRatio;
} camera;

#define MAX_PARTICLES_MEDIUM 1000

struct particle {
   vec2 position;
   vec2 velocity;
   float scale;
   float life;
};

struct ParticleSystemConfiguration {
   int particleCount;
   bool burstMode;
   float spawnRate; 
   float particleLifeSpan;
   float gravity;
   float startSize; 
   float endSize;
};

struct particleSystem {
   ParticleSystemConfiguration configuration;
   particle particles[MAX_PARTICLES_MEDIUM];
};

layout(std430, set = 0, binding = 0) readonly buffer ObjectInstaceBuffer{
	particleSystem ssboData[];
};

layout(push_constant) uniform constants{
   int systemIndex;
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
    view *= translate(vec2(-camera.position.x, camera.position.y));
    view *= scale(vec2(1.0, -1.0));

    mat4 model = mat4(1.0);
    model *= translate(ssboData[systemIndex].particles[gl_InstanceIndex].position);
   //  model *= rotate(ssboData[systemIndex].rotation);
    model *= scale(vec2(ssboData[systemIndex].particles[gl_InstanceIndex].scale) * step(0.0, ssboData[systemIndex].particles[gl_InstanceIndex].life)); // step SHOULD hide dead particles

    gl_Position = view * model  * vec4(inPosition, 0.0, 1.0) * vec4(camera.aspectRatio, 1.0, 1.0, 1.0);

    instance_index = gl_InstanceIndex;
    uv = vec2(inFragCoord.x, 1.0 - inFragCoord.y);
}