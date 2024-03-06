#version 460
#extension GL_EXT_nonuniform_qualifier : enable

struct ssboObject{
   vec2 uvMin;
   vec2 uvMax;
   vec2 translation;
   vec2 scale;
   float rotation;
   int useLightMap;
   int index; // texture index
};

layout(set = 1, binding = 2) uniform LightMapUBO {
   int lightMapIndex;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectInstaceBuffer{
	ssboObject ssboData[];
} ssboBuffer;


layout(binding = 0) uniform sampler2D texSampler[];

layout(location = 2) in flat int instance_index;
layout(location = 1) in vec2 uv;
layout(location = 3) in vec2 screenSpaceUV;
layout(location = 0) out vec4 outColor;

void main() {

   vec2 umin = ssboBuffer.ssboData[instance_index].uvMin;
   vec2 umax = ssboBuffer.ssboData[instance_index].uvMax;

   float xscale = (umax.x - umin.x);
   float sampleX = umin.x + xscale * (1.0 - uv.x);

   float yscale = (umax.y - umin.y);
   float sampleY = umin.y + yscale * (uv.y);

   vec4 sampleColor = texture(texSampler[ssboBuffer.ssboData[instance_index].index], vec2( sampleX, sampleY));

   float brightness = texture(texSampler[lightMapIndex], screenSpaceUV).r;
   outColor = vec4(sampleColor.rgb * brightness, sampleColor.a);
}