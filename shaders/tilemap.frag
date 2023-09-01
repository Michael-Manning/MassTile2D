#version 460
precision highp float;

layout(binding = 1) uniform sampler2D atlasTexture;
#define atlasCount 32;

layout(std430, set = 1, binding = 0) readonly buffer ObjectBuffer{
	int ssboData[];
} ;

layout(push_constant) uniform constants {
   int mapw;
   int maph;
} ;

layout(location = 2) in flat int instance_index;
layout(location = 1) in vec2 uv;
layout(location = 0) out vec4 outColor;

// #define chunkSize 32
// #define chunkTileCount 1024
// #define chunksX  128

#define chunkSize 32
#define chunkTileCount 1024
#define chunksX  64 

int getTile(int x, int y) {
    int cx = (x / chunkSize);
    int cy = (y / chunkSize);
    int chunk = cy * chunksX + cx;
    int chuckIndexOffset = chunk * chunkTileCount;
    return ssboData[chuckIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize)];
};

void main() {
    int xi = int(uv.x * mapw);
    xi = min(xi, mapw - 1);

    int yi = int(uv.y * maph);
    yi = min(yi, maph - 1);

    int atlasIndex = getTile(xi, yi);

    int atlasX = atlasIndex % atlasCount;
    int atlasY = atlasIndex / atlasCount;

    float atlasUvSize = 1.0 / atlasCount;

    vec2 mapUvSize = vec2(1.0 / float(mapw), 1.0 / float(maph));   

    vec2 localUV = vec2(fract(uv.x * mapw), mod(uv.y * maph, 1.0));
    vec2 umin = localUV * atlasUvSize + vec2(float(atlasX), float(atlasY)) * atlasUvSize;

    outColor = texture(atlasTexture, umin);
}

