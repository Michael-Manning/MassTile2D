
#pragma once

#include <stdint>
#include <glm/glm.hpp>
#include <std140.h>

namespace ShaderTypes
{
	/*
		Shader structure ColoredQuadSSBOObject
		Type size: 40
		Array stride: 48
	*/
	struct alignas(16) ColoredQuadSSBOObject
	{
		glm::vec4 color;
		glm::vec2 position;
		glm::vec2 scale;
		int32_t   circle;
		float     rotation;
	};

	/*
		Storage buffer ColoredQuadInstaceBuffer
		Type size: 40
	*/
	struct ColoredQuadInstaceBuffer
	{
		ColoredQuadSSBOObject instanceData[];
	};

	/*
		Uniform buffer CamerUBO
		Type size: 16
	*/
	struct CamerUBO
	{
		glm::vec2 position;
		float     zoom;
		float     aspectRatio;
	};

	/*
		Shader structure ColoredTriangleSSBOObject
		Type size: 16
		Array stride: 16
	*/
	struct ColoredTriangleSSBOObject
	{
		glm::vec4 color;
	};

	/*
		Storage buffer ColoredTriangleInstaceBuffer
		Type size: 16
	*/
	struct ColoredTriangleInstaceBuffer
	{
		ColoredTriangleSSBOObject ssboData[];
	};

	/*
		Storage buffer mapFGObjectBuffer
		Type size: 0
	*/
	struct mapFGObjectBuffer
	{
		int32_t tileMapFGData[];
	};

	/*
		Shader structure LightingUpdate
		Type size: 1616
		Array stride: 1616
	*/
	struct LightingUpdate
	{
		int32_t                      chunkIndex;
		int32_t                      lightCount;
		alignas(16) std140_vec2Array lightPositions[100];
	};

	/*
		Storage buffer baseLightingObjectBuffer
		Type size: 1616
	*/
	struct baseLightingObjectBuffer
	{
		LightingUpdate baseLightingUpdates[];
	};

	/*
		Storage buffer blueLightingObjectBuffer
		Type size: 1616
	*/
	struct blueLightingObjectBuffer
	{
		LightingUpdate blurLightingUpdates[];
	};

	/*
		Storage buffer mapBGObjectBuffer
		Type size: 0
	*/
	struct mapBGObjectBuffer
	{
		int32_t tileMapBGData[];
	};

	/*
		Storage buffer mapUpscaleObjectBuffer
		Type size: 0
	*/
	struct mapUpscaleObjectBuffer
	{
		int32_t tileMapUpscaleData[];
	};

	/*
		Storage buffer mapBlurObjectBuffer
		Type size: 0
	*/
	struct mapBlurObjectBuffer
	{
		int32_t tileMapBlurData[];
	};

	/*
		Shader structure ParticleSystemConfiguration
		Type size: 64
		Array stride: 64
	*/
	struct ParticleSystemConfiguration
	{
		int32_t   particleCount;
		uint32_t  burstMode;
		uint32_t  burstRepeate;
		float     spawnRate;
		float     particleLifeSpan;
		float     gravity;
		float     startSize;
		float     endSize;
		glm::vec4 startColor;
		glm::vec4 endColor;
	};

	/*
		Storage buffer ParticalInstanceConfigBuffer
		Type size: 64
	*/
	struct ParticalInstanceConfigBuffer
	{
		ParticleSystemConfiguration configs[];
	};

	/*
		Shader structure Particle
		Type size: 48
		Array stride: 48
	*/
	struct Particle
	{
		glm::vec2             position;
		glm::vec2             velocity;
		float                 scale;
		float                 life;
		alignas(16) glm::vec4 color;
	};

	/*
		Shader structure ParticleGroup_large
		Type size: 19200000
		Array stride: 19200000
	*/
	struct ParticleGroup_large
	{
		Particle particles[400000];
	};

	/*
		Storage buffer ParticalLargeGroupInstanceBuffer
		Type size: 19200000
	*/
	struct ParticalLargeGroupInstanceBuffer
	{
		ParticleGroup_large particleGroups_large[];
	};

	/*
		Storage buffer AtomicCounterBuffer
		Type size: 4
	*/
	struct AtomicCounterBuffer
	{
		uint32_t activeCount;
	};

	/*
		Shader structure ParticleGroup_small
		Type size: 19200
		Array stride: 19200
	*/
	struct ParticleGroup_small
	{
		Particle particles[400];
	};

	/*
		Storage buffer ParticalSmallGroupInstanceBuffer
		Type size: 192000
	*/
	struct ParticalSmallGroupInstanceBuffer
	{
		ParticleGroup_small particleGroups_small[10];
	};

	/*
		Storage buffer ObjectInstaceBuffer_large
		Type size: 19200000
	*/
	struct ObjectInstaceBuffer_large
	{
		ParticleGroup_large particleGroups_large[];
	};

	/*
		Shader structure TextureSSBOObject
		Type size: 44
		Array stride: 48
	*/
	struct alignas(16) TextureSSBOObject
	{
		glm::vec2 uvMin;
		glm::vec2 uvMax;
		glm::vec2 translation;
		glm::vec2 scale;
		float     rotation;
		int32_t   useLightMap;
		int32_t   index;
	};

	/*
		Storage buffer TextureInstaceBuffer
		Type size: 44
	*/
	struct TextureInstaceBuffer
	{
		TextureSSBOObject instanceData[];
	};

	/*
		Uniform buffer LightMapUBO
		Type size: 4
	*/
	struct LightMapUBO
	{
		int32_t lightMapIndex;
	};

	/*
		Shader structure TextHeader
		Type size: 44
		Array stride: 48
	*/
	struct alignas(16) TextHeader
	{
		glm::vec4 color;
		glm::vec2 position;
		glm::vec2 scale;
		float     rotation;
		int32_t   _textureIndex;
		int32_t   textLength;
	};

	/*
		Storage buffer TextHeaderInstaceBuffer
		Type size: 44
	*/
	struct TextHeaderInstaceBuffer
	{
		TextHeader headerData[];
	};

	/*
		Shader structure CharQuad
		Type size: 32
		Array stride: 32
	*/
	struct CharQuad
	{
		glm::vec2 uvmin;
		glm::vec2 uvmax;
		glm::vec2 scale;
		glm::vec2 position;
	};

	/*
		Storage buffer TextDataInstaceBuffer
		Type size: 32
	*/
	struct TextDataInstaceBuffer
	{
		CharQuad textData[];
	};

	/*
		Shader structure LetterIndexInfo
		Type size: 8
		Array stride: 16
	*/
	struct LetterIndexInfo
	{
		uint32_t headerIndex;
		uint32_t letterIndex;
	};

	/*
		Storage buffer TextIndexInstaceBuffer
		Type size: 8
	*/
	struct TextIndexInstaceBuffer
	{
		LetterIndexInfo indexData[];
	};


} // namespace
    