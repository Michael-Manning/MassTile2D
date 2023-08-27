#version 460

// change camera to push constant

layout(location = 0) in vec2 inPosition;
layout(location = 2) in vec2 inFragCoord;

layout(binding = 0) uniform UniformBufferObject {
   float aspect;
} ubo;

layout(push_constant) uniform constants {
   vec2 cameraTranslation;
   float cameraZoom;
} pConstants;

struct ssboObject{
   vec2 uvMin;
   vec2 uvMax;
   vec2 translation;
   vec2 scale;
   float rotation;

   int index;
};
layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer{
	ssboObject ssboData[];
} ssboBuffer;

// layout(location = 3) in vec2 instancePosition;

layout(location = 1) out vec2 uv;
layout(location = 2) out flat int instance_index;


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
   

   mat4 view = mat4(1.0);
   view *= scale(vec2(pConstants.cameraZoom));
   view *= translate(vec2(-pConstants.cameraTranslation.x, pConstants.cameraTranslation.y));
   view *= scale(vec2(1.0, -1.0));

   mat4 model = mat4(1.0);
   model *= translate(ssboBuffer.ssboData[gl_InstanceIndex].translation);
   model *= rotate(ssboBuffer.ssboData[gl_InstanceIndex].rotation);
   model *= scale(ssboBuffer.ssboData[gl_InstanceIndex].scale);
   
//model = ssboBuffer.ssboData[gl_InstanceIndex].model;

   // gl_Position = view * model  * vec4(inPosition, 0.0, 1.0) * vec4(vec2( ubo.aspect, 1.0), 1.0, 1.0);
   //gl_Position = vec4(inPosition, 0.0, 1.0) * vec4(vec2( ubo.aspect, 1.0), 1.0, 1.0);

   gl_Position = view * model  * vec4(inPosition, 0.0, 1.0) * vec4(vec2( ubo.aspect, 1.0), 1.0, 1.0);
   //  gl_Position = ssboBuffer.ssboData[gl_InstanceIndex].view * ssboBuffer.ssboData[gl_InstanceIndex].model  * vec4(inPosition, 0.0, 1.0) * vec4(vec2( ubo.aspect, 1.0), 1.0, 1.0);

   instance_index = gl_InstanceIndex;
   uv = vec2(inFragCoord.x, 1.0 - inFragCoord.y);
}