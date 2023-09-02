#version 450

struct ssboObject{
   vec4 color;
   vec2 position;
   vec2 scale;
   int circle;
   float rotation;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectInstaceBuffer{
	ssboObject ssboData[];
};


// layout(push_constant) uniform constants {
//    mat4 model;
//    vec4 color;
//    int circle;
// };

layout(location = 2) in flat int instance_index;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 outColor;

void main() {
   vec4 col = ssboData[instance_index].color;
   if( ssboData[instance_index].circle == 1 && length(uv - 0.5) > 0.5){
      col.a = 0.0;
   }
   outColor = col;
}