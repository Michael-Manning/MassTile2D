#version 460

layout(location = 0) in vec2 inPosition;
layout(location = 2) in vec2 inFragCoord;

layout(set = 1, binding = 0) uniform CamerUBO {
   vec2 position;
	float zoom;
	float aspectRatio;
} camera;

struct CharQuad {
	vec2 uvmin;
	vec2 uvmax;
	vec2 scale;
	vec2 position;
};

struct TextHeader {
   vec4 color;
   vec2 position;
   vec2 scale;
   float rotation;
   int _textureIndex;
   int textLength;
};

struct LetterIndexInfo {
   uint headerIndex;
   uint letterIndex;
};

layout(std140, set = 1, binding = 1) readonly buffer TextHeaderInstaceBuffer{
	TextHeader headerData[];
};

layout(std430, set = 1, binding = 2) readonly buffer TextDataInstaceBuffer{
	CharQuad textData[];
};

layout(std140, set = 1, binding = 3) readonly buffer TextIndexInstaceBuffer{
	LetterIndexInfo indexData[];
};

layout(location = 1) out vec2 uv;
layout(location = 2) out flat uint obj_index;
layout(location = 3) out flat uint letter_index;

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

   // int indexAccumulator = 0;
   // int i = 0;
   // for(; i < TEXTPL_maxTextObjects; i++){
   //    if(gl_InstanceIndex < (indexAccumulator + headerData[i].textLength)){
   //       break;
   //    }
   //    indexAccumulator += headerData[i].textLength;
   // }

   // obj_index = i;
   // letter_index = gl_InstanceIndex - indexAccumulator;

   obj_index = indexData[gl_InstanceIndex].headerIndex;
   letter_index = indexData[gl_InstanceIndex].letterIndex;

   
   mat4 view = mat4(1.0);
   view *= scale(vec2(camera.zoom));
   view *= translate(camera.position);

   mat4 letterModel = mat4(1.0);
   letterModel *= translate(textData[letter_index].position);
   letterModel *= scale(textData[letter_index].scale );

   mat4 objectModel = mat4(1.0);
   objectModel *= translate(headerData[obj_index].position);
   objectModel *= rotate(headerData[obj_index].rotation);
   objectModel *= scale(headerData[obj_index].scale);

   gl_Position = view * objectModel * letterModel  * vec4(inPosition, 0.0, 1.0) * vec4(vec2( camera.aspectRatio, 1.0), 1.0, 1.0);

   uv = vec2(inFragCoord.x, inFragCoord.y);
}