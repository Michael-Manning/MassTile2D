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

float sampleBrightnessGrid(int x, int y){
    return float(getTile(x, y) >> 16) / 255.0;
}

// float x = uv.x * gridW;
// float y = uv.y * gridH;

// float gridX = int(x);
// float gridY = int(y);

// float brightness = getTile(gridX, gridY);

void main() {
    int xi = int(uv.x * mapw);
    xi = min(xi, mapw - 1);

    int yi = int(uv.y * maph);
    yi = min(yi, maph - 1);

    int bufferValue = getTile(xi, yi);
    int atlasIndex = bufferValue & 0xFFFF;

    int atlasX = atlasIndex % atlasCount;
    int atlasY = atlasIndex / atlasCount;

    float atlasUvSize = 1.0 / atlasCount;

    vec2 mapUvSize = vec2(1.0 / float(mapw), 1.0 / float(maph));   

    vec2 localUV = vec2(fract(uv.x * mapw), mod(uv.y * maph, 1.0));
    vec2 umin = localUV * atlasUvSize + vec2(float(atlasX), float(atlasY)) * atlasUvSize;

    vec4 col = texture(atlasTexture, umin);

    float brightness;

    //switch to sampling with padding to avoid branching
    if(xi > 1 && xi < mapw - 1 && yi > 1 && yi < maph - 1){

        int tile00 = getTile(xi, yi);
        float brightness00 = tile00 >> 16;
        bool air = (tile00 & 0xFFFF) == 1023;
        int sx = int(sign(2.0 * localUV.x - 1.0));
        int sy = int(sign(2.0 * localUV.y - 1.0));

        int tile10 = getTile(xi + sx, yi);
        int tile01 = getTile(xi, yi + sy);
        int tile11 = getTile(xi + sx, yi + sy);

        float brightness10 = (air && ((tile10 & 0xFFFF) != 1023)) ? brightness00 : (tile10 >> 16);
        float brightness01 = (air && ((tile01 & 0xFFFF) != 1023)) ? brightness00 : (tile01 >> 16);
        float brightness11 = (air && ((tile11 & 0xFFFF) != 1023)) ? brightness00 : (tile11 >> 16);

        // float brightness10 = sampleBrightnessGrid(xi + sx, yi);
        // float brightness01 = sampleBrightnessGrid(xi, yi + sy);
        // float brightness11 = sampleBrightnessGrid(xi + sx, yi + sy);

        float brightness0 = mix(brightness00, brightness10, abs(localUV.x - 0.5));
        float brightness1 = mix(brightness01, brightness11, abs(localUV.x - 0.5));
        brightness = mix(brightness0, brightness1, abs(localUV.y - 0.5)) / 255.0;
    }
    else{
         brightness = sampleBrightnessGrid(xi, yi);
    }

    outColor = vec4(col.rgb * brightness, col.a);
}

