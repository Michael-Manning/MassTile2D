#version 460
#extension GL_EXT_nonuniform_qualifier : enable
precision highp float;

#define atlasCount 32;

layout(std430, set = 1, binding = 1) buffer mapFGObjectBuffer {
	int tileMapFGData[];
} ;
layout(std430, set = 1, binding = 2) buffer mapBGObjectBuffer {
	int tileMapBGData[];
} ;
layout(std430, set = 1, binding = 3) readonly buffer mapUpscaleObjectBuffer {
	int tileMapUpscaleData[];
} ;
layout(std430, set = 1, binding = 4) buffer mapBlurObjectBuffer {
	int tileMapBlurData[];
} ;

layout(push_constant) uniform constants{
	int interpolationEnabled;
	int upscaleEnabled;
	int blurEnabled;
};

layout(binding = 0) uniform sampler2D texSampler[];

#define mapw 2048
#define maph 1024
#define mapOffset 0
#define mapCount 2097152

#define chunkSize2 64
#define chunkTileCount2 4096
#define chunksX2  64 
#define mapw2 4096
#define maph2 2048
#define mapCount2 8388608

layout(location = 2) in flat int instance_index;
layout(location = 1) in vec2 uv;
layout(location = 0) out vec4 outColor;


#define chunkSize 32
#define chunkTileCount 1024
#define chunksX  64 

int getTile(int x, int y) {
   int cx = (x / chunkSize);
   int cy = (y / chunkSize);
   int chunk = cy * chunksX + cx;
   int chuckIndexOffset = chunk * chunkTileCount;
   uint index = uint(mapOffset + chuckIndexOffset + (y % chunkSize) * chunkSize + (x % chunkSize));
   return tileMapFGData[index % mapCount];
};

float sampleBrightnessGrid(int x, int y){
   //return float(getTile(x, y) >> 16) ;
   
   int cx = (x / chunkSize2);
   int cy = (y / chunkSize2);
   int chunk = cy * chunksX + cx;
   int chuckIndexOffset = chunk * chunkTileCount2;
   uint index = uint( chuckIndexOffset + (y % chunkSize2) * chunkSize2 + (x % chunkSize2));

   if(blurEnabled == 1)
      return tileMapBlurData[index % mapCount2];
   return tileMapUpscaleData[index % mapCount2];
}



void main() {

   int xi = int(uv.x * mapw);
   xi = min(xi, mapw - 1);
   int yi = int(uv.y * maph);
   yi = min(yi, maph - 1);

   if(upscaleEnabled == 0){

      if(interpolationEnabled == 0){
         outColor = vec4( (getTile(xi, yi) >> 16) / 255.0, 0.0, 0.0, 0.0);
         return;
      }

      vec2 localUV = vec2(fract(uv.x * mapw), mod(uv.y * maph, 1.0));

      float brightness;

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

      float brightness0 = mix(brightness00, brightness10, abs(localUV.x - 0.5));
      float brightness1 = mix(brightness01, brightness11, abs(localUV.x - 0.5));
      brightness = mix(brightness0, brightness1, abs(localUV.y - 0.5)) / 255.0;
      
      outColor = vec4(brightness, 0.0, 0.0, 0.0);
      return;
   }



   int xi2 = int(uv.x * mapw2);
   xi2 = min(xi2, mapw2 - 1);
   int yi2 = int(uv.y * maph2);
   yi2 = min(yi2, maph2 - 1);

   if(interpolationEnabled == 0){
      outColor = vec4(sampleBrightnessGrid(xi2, yi2)/ 255.0, 0.0, 0.0, 0.0);
      return;
   }

   vec2 localUV = vec2(fract(uv.x * mapw2), mod(uv.y * maph2, 1.0));

   float brightness;


   if(xi > 1 && xi < mapw - 1 && yi > 1 && yi < maph - 1){

      int tile00 = getTile(xi, yi);
      float brightness00 = sampleBrightnessGrid(xi2, yi2);
      float brightnessbase = brightness00; // tile00 >> 16;
      bool air = (tile00 & 0xFFFF) == 1023;
      int sx = int(sign(2.0 * localUV.x - 1.0));
      int sy = int(sign(2.0 * localUV.y - 1.0));

     int tile10 = getTile(xi + sx, yi);
     int tile01 = getTile(xi, yi + sy);
     int tile11 = getTile(xi + sx, yi + sy);

     float brightness10 = (air && ((tile10 & 0xFFFF) != 1023)) ? brightnessbase : sampleBrightnessGrid(xi2 + sx, yi2);
     float brightness01 = (air && ((tile01 & 0xFFFF) != 1023)) ? brightnessbase : sampleBrightnessGrid(xi2, yi2 + sy);
     float brightness11 = (air && ((tile11 & 0xFFFF) != 1023)) ? brightnessbase : sampleBrightnessGrid(xi2 + sx, yi2 + sy);

      // float brightness10 = sampleBrightnessGrid(xi2 + sx, yi2);
      // float brightness01 = sampleBrightnessGrid(xi2, yi2 + sy);
      // float brightness11 = sampleBrightnessGrid(xi2 + sx, yi2 + sy);

      float brightness0 = mix(brightness00, brightness10, abs(localUV.x - 0.5));
      float brightness1 = mix(brightness01, brightness11, abs(localUV.x - 0.5));
      brightness = mix(brightness0, brightness1, abs(localUV.y - 0.5)) / 255.0;
   }
   else{
     brightness = sampleBrightnessGrid(xi2, yi2)/ 255.0;
   }

   outColor = vec4(brightness, 0.0, 0.0, 0.0);
}






// void main() {
//    int xi = int(uv.x * mapw);
//    xi = min(xi, mapw - 1);

//    int yi = int(uv.y * maph);
//    yi = min(yi, maph - 1);

//    vec4 fgCol;
//    vec4 bgCol;

//    vec2 localUV = vec2(fract(uv.x * mapw), mod(uv.y * maph, 1.0));


//    float brightness;

//    //switch to sampling with padding to avoid branching
//    if(xi > 1 && xi < mapw - 1 && yi > 1 && yi < maph - 1){

//       int tile00 = getTile(xi, yi);
//       float brightness00 = tile00 >> 16;
//       bool air = (tile00 & 0xFFFF) == 1023;
//       int sx = int(sign(2.0 * localUV.x - 1.0));
//       int sy = int(sign(2.0 * localUV.y - 1.0));

//      int tile10 = getTile(xi + sx, yi);
//      int tile01 = getTile(xi, yi + sy);
//      int tile11 = getTile(xi + sx, yi + sy);

//      float brightness10 = (air && ((tile10 & 0xFFFF) != 1023)) ? brightness00 : (tile10 >> 16);
//      float brightness01 = (air && ((tile01 & 0xFFFF) != 1023)) ? brightness00 : (tile01 >> 16);
//      float brightness11 = (air && ((tile11 & 0xFFFF) != 1023)) ? brightness00 : (tile11 >> 16);

//       // float brightness10 = sampleBrightnessGrid(xi + sx, yi);
//       // float brightness01 = sampleBrightnessGrid(xi, yi + sy);
//       // float brightness11 = sampleBrightnessGrid(xi + sx, yi + sy);

//       float brightness0 = mix(brightness00, brightness10, abs(localUV.x - 0.5));
//       float brightness1 = mix(brightness01, brightness11, abs(localUV.x - 0.5));
//       brightness = mix(brightness0, brightness1, abs(localUV.y - 0.5)) / 255.0;
//    }
//    else{
//       brightness = sampleBrightnessGrid(xi, yi);
//    }
   
//    outColor = vec4(brightness, 0.0, 0.0, 0.0);
// }

