#version 450

layout(location = 1) in vec2 uv;

layout(location = 2) in flat int instance_index;

layout(location = 0) out vec4 outColor;

struct ssboObject{
   vec4 color;
};

layout(std140, set = 0, binding = 1) readonly buffer ObjectInstaceBuffer{
	ssboObject ssboData[];
};


void main() {

   outColor = ssboData[instance_index].color;
   // outColor = vec4(1.0, 1.0, 1.0, 1.0);
}