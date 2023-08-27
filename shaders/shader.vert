#version 450

layout(push_constant) uniform constants {
   // mat4 model;
   // mat4 view;
   vec2 cameraTranslation;
   float cameraZoom;
   vec2 translation;
   vec2 scale;
   float rotation;
   int index;
} pConstants;

layout(binding = 0) uniform UniformBufferObject {
   float aspect;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 2) in vec2 inFragCoord;

layout(location = 1) out vec2 uv;

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
   model *= translate(pConstants.translation);
   model *= rotate(pConstants.rotation);
   model *= scale(pConstants.scale);

   gl_Position = view * model * vec4(inPosition, 0.0, 1.0) * vec4(vec2( ubo.aspect, 1.0), 1.0, 1.0);
   // gl_Position = pConstants.view * pConstants.model * vec4(inPosition, 0.0, 1.0) * vec4(vec2( ubo.aspect, 1.0), 1.0, 1.0);
 //  uv = inFragCoord;
   uv = vec2(inFragCoord.x, 1.0 - inFragCoord.y);
}