#version 450

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

layout(location = 1) in vec2 uv;
layout(location = 2) in flat int instance_index;
layout(location = 0) out vec4 outColor;

void main() {
   outColor = vec4(1.0, 1.0, 0.0, 1.0 - length((uv - 0.5) * 2.0));
}