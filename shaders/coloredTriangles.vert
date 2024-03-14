#version 450

layout(binding = 0) uniform CamerUBO {
    vec2 position;
    float zoom;
    float aspectRatio;
} camera;

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

    mat4 view = mat4(1.0);
    view *= scale(vec2(camera.zoom));
    view *= translate(camera.position);

    gl_Position = view * vec4(inPosition, 0.0, 1.0) * vec4(camera.aspectRatio, 1.0, 1.0, 1.0);
    
    uv = vec2(inFragCoord.x, 1.0 - inFragCoord.y);
    instance_index = gl_VertexIndex / 3;
}