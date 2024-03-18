#version 460

#define TEXTPL_maxTextObjects 100
#define TEXTPL_maxTextLength 128


layout(location = 0) in vec2 inPosition;
layout(location = 2) in vec2 inFragCoord;

layout(set = 1, binding = 1) uniform CamerUBO {
   vec2 position;
	float zoom;
	float aspectRatio;
} camera;


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
   vec4 color;
   vec2 position;
   vec2 scale;
   float rotation;
   int _textureIndex;
   int textLength;
};
struct textIndexes_ssbo {
   textHeader headers[TEXTPL_maxTextObjects];
   textObject textData[TEXTPL_maxTextObjects];
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectInstaceBuffer{
	textIndexes_ssbo ssboData;
};

layout(location = 1) out vec2 uv;
layout(location = 2) out flat int obj_index;
layout(location = 3) out flat int letter_index;

mat4 translate(vec2 v) {
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        v.x, v.y, 0.0, 1.0
    );
}
mat4 scale(vec2 v) {
    return mat4(
        v.x, 0.0, 0.0, 0.0,
        0.0, v.y, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}
mat4 rotate(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat4(
        c,   s,   0.0, 0.0,
       -s,   c,   0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

void main() {

   int indexAccumulator = 0;
   int i = 0;
   for(; i < TEXTPL_maxTextObjects; i++){
      if(gl_InstanceIndex < (indexAccumulator + ssboData.headers[i].textLength)){
         break;
      }
      indexAccumulator += ssboData.headers[i].textLength;
   }

   obj_index = i;
   letter_index = gl_InstanceIndex - indexAccumulator;

   
   mat4 view = mat4(1.0);
   view *= scale(vec2(camera.zoom));
   view *= translate(camera.position);

   mat4 letterModel = mat4(1.0);
   letterModel *= translate(ssboData.textData[i].quads[letter_index].position * vec2(1.0, 1.0));
   letterModel *= scale(ssboData.textData[i].quads[letter_index].scale );

   mat4 objectModel = mat4(1.0);
   objectModel *= translate(ssboData.headers[i].position);
   objectModel *= rotate(ssboData.headers[i].rotation);
   objectModel *= scale(ssboData.headers[i].scale);

   gl_Position = view * objectModel * letterModel  * vec4(inPosition, 0.0, 1.0) * vec4(vec2( camera.aspectRatio, 1.0), 1.0, 1.0);

   uv = vec2(inFragCoord.x, inFragCoord.y);
}