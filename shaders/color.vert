#version 450

layout(push_constant) uniform constants {
   mat4 model;
   mat4 view;
   vec4 color;
   int circle;
} pConstants;

layout(binding = 0) uniform UniformBufferObject {
   float aspect;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 2) in vec2 inFragCoord;

layout(location = 1) out vec2 uv;


void main() {
   gl_Position = pConstants.view * pConstants.model * vec4(inPosition, 0.0, 1.0) * vec4(vec2( ubo.aspect, 1.0), 1.0, 1.0);
   
   uv = inFragCoord;
}