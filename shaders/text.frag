#version 460
#extension GL_EXT_nonuniform_qualifier : enable

struct charQuad {
	vec2 uvmin;
	vec2 uvmax;
	vec2 scale;
	vec2 position;
};

struct textHeader {
   vec4 color;
   vec2 position;
   vec2 scale;
   float rotation;
   int _textureIndex;
   int textLength;
};

layout(std140, set = 1, binding = 1) readonly buffer headerInstaceBuffer{
	textHeader headerData[];
};

layout(std430, set = 1, binding = 2) readonly buffer textDataInstaceBuffer{
	charQuad textData[];
};

layout(set = 0, binding = 0) uniform sampler2D texSampler[];

layout(location = 1) in vec2 uv;
layout(location = 2) in flat uint obj_index;
layout(location = 3) in flat uint letter_index;

layout(location = 0) out vec4 outColor;

void main() {

   vec2 umin = textData[letter_index].uvmin;
   vec2 umax = textData[letter_index].uvmax;

   float xscale = (umax.x - umin.x);
   float sampleX = umin.x + xscale * (uv.x);

   float yscale = (umax.y - umin.y);
   float sampleY = umin.y + yscale * (uv.y);

   float alpha = texture(texSampler[headerData[obj_index]._textureIndex], vec2( sampleX, sampleY)).x;
   vec4 col = headerData[obj_index].color;

   col = vec4(col.rgb, mix(0.0, col.a, alpha));
   //col.a = max(col.a, 0.3);
   outColor = col;
}