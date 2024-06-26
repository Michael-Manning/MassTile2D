#version 450

#define MAX_PARTICLES_SMALL 400
#define MAX_PARTICLES_LARGE 400000

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
	ParticleGroup_small particleGroups_small[];
};

layout(std430, set = 0, binding = 2) buffer ObjectInstaceBuffer_large{
	ParticleGroup_large particleGroups_large[MAX_PARTICLE_SYSTEMS_LARGE];
};

layout(push_constant) uniform constants{
   int systemIndex;
   int systemSize; // small = 0, large  = 1;
};

layout(location = 1) in vec2 uv;
layout(location = 2) in flat int instance_index;
layout(location = 0) out vec4 outColor;

void main() {

   particle p = systemSize == 0 ? particleGroups_small[systemIndex].particles[instance_index] : particleGroups_large[systemIndex].particles[instance_index];

   outColor = vec4(p.color.r, p.color.g, p.color.b, p.color.a - length((uv - 0.5) * 2.0));
   // outColor = vec4(1.0, 1.0, 0.0, 1.0 - length((uv - 0.5) * 2.0));
}