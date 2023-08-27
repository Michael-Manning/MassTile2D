#version 450

layout(push_constant) uniform constants {
   //  mat4 model;
   //  mat4 view;
   vec2 cameraTranslation;
   float cameraZoom;
   vec2 translation;
   vec2 scale;
   float rotation;
   int index;
} pConstants;


#define MAX_TEXTURES 10
layout(binding = 1) uniform sampler2D texSampler[MAX_TEXTURES];

layout(location = 1) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main() {
   outColor = texture(texSampler[pConstants.index], vec2( 1.0 - uv.x, uv.y));
}