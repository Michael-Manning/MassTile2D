#version 450

layout(push_constant) uniform constants {
   mat4 model;
   mat4 view;
   vec4 color;
   int circle;
} pConstants;

layout(location = 1) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main() {
   vec4 col = pConstants.color;
   if(pConstants.circle == 1 && length(uv - 0.5) > 0.5){
      col.a = 0.0;
   }
   outColor = col;
}