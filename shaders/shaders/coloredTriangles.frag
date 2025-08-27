#version 450

layout(location = 2) in flat int instance_index;

layout(location = 0) out vec4 outColor;

struct ColoredTriangleSSBOObject{
   vec4 color;
};

layout(std140, set = 0, binding = 1) readonly buffer ColoredTriangleInstaceBuffer{
	ColoredTriangleSSBOObject ssboData[];
};

void main() {
   outColor = ssboData[instance_index].color;
}