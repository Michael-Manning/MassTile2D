#version 460

#define TEXTPL_maxFonts 10
#define TEXTPL_maxTextObjects 10
#define TEXTPL_maxTextLength 128

struct charQuad {
	vec2 uvmin;
	vec2 uvmax;
	vec2 scale;
	vec2 position;
};
struct textObject {
   charQuad quads[TEXTPL_maxTextLength];
};
struct textHeader {
   int _textureIndex;
   int textLength;
   vec2 position;
   float scale;
   float rotation;
   vec4 color;
};
struct textIndexes_ssbo {
   textHeader headers[TEXTPL_maxTextObjects];
   textObject textData[TEXTPL_maxTextObjects];
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectInstaceBuffer{
	textIndexes_ssbo ssboData;
};


layout(binding = 1) uniform sampler2D texSampler[TEXTPL_maxFonts];

layout(location = 1) in vec2 uv;
layout(location = 2) in flat int obj_index;
layout(location = 3) in flat int letter_index;

layout(location = 0) out vec4 outColor;

void main() {

   vec2 umin = ssboData.textData[obj_index].quads[letter_index].uvmin;
   vec2 umax = ssboData.textData[obj_index].quads[letter_index].uvmax;

   float xscale = (umax.x - umin.x);
   float sampleX = umin.x + xscale * (1.0 - uv.x);

   float yscale = (umax.y - umin.y);
   float sampleY = umin.y + yscale * (uv.y);

   vec4 col = texture(texSampler[ssboData.headers[obj_index]._textureIndex], vec2( sampleX, sampleY));
   outColor = vec4(col.rgb, 1.0);

   // outColor = texture(texSampler[ssboData.headers[obj_index]._textureIndex], vec2( sampleX, sampleY));
}