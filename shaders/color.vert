#version 450

layout(binding = 0) uniform CamerUBO {
   vec2 position;
	float zoom;
	float aspectRatio;

   mat3 view;
} camera;

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

struct transformObject{
   mat3 transform;
};
layout(std430, set = 0, binding = 1) readonly buffer ObjectTransformBufferr{
	transformObject transformData[];
};


layout(location = 0) in vec2 inPosition;
layout(location = 2) in vec2 inFragCoord;

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

   // mat4 view = mat4(1.0);
   // view *= scale(vec2(camera.zoom));
   // view *= translate(vec2(-camera.position.x, camera.position.y));
   // view *= scale(vec2(1.0, -1.0));

   // mat4 model = mat4(1.0);
   // model *= translate(ssboData[gl_InstanceIndex].position);
   // model *= rotate(ssboData[gl_InstanceIndex].rotation);
   // model *= scale(ssboData[gl_InstanceIndex].scale);

   // gl_Position = view * model  * vec4(inPosition, 0.0, 1.0) * vec4(vec2( camera.aspectRatio, 1.0), 1.0, 1.0);

   gl_Position = vec4(transformData[gl_InstanceIndex].transform * vec3(inPosition, 1.0) * vec3(camera.aspectRatio, 1.0, 1.0), 1.0);

   instance_index = gl_InstanceIndex;
   uv = vec2(inFragCoord.x, 1.0 - inFragCoord.y);
}