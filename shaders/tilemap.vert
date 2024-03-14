#version 460
precision highp float;

layout(set = 1, binding = 0) uniform CamerUBO {
    vec2 position;
    float zoom;
    float aspectRatio;
} camera;

layout(location = 0) in vec2 inPosition;
layout(location = 2) in vec2 inFragCoord;

layout(location = 1) out vec2 uv;
layout(location = 3) out vec2 screenSpaceUV;
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
   view *= scale(vec2(camera.zoom));
   view *= translate(camera.position);

   mat4 model = mat4(1.0);
   model *= translate(vec2(0.0));
   model *= rotate(0.0);
   model *= scale(vec2(512, 256));

   gl_Position = view * model  * vec4(inPosition, 0.0, 1.0) * vec4(vec2( camera.aspectRatio, 1.0), 1.0, 1.0);

   instance_index = gl_InstanceIndex;
   uv = vec2(1.0 - inFragCoord.x, inFragCoord.y);
   screenSpaceUV = (gl_Position.xy + vec2(1.0)) / 2.0; // Map from [-1, 1] to [0, 1]
}