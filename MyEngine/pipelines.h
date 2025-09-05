#pragma once


//#include "texturedQuadPL.h"
//#include "tilemapPL.h"
//#include "TilemapLightRasterPL.h"
//#include "LightingComputePL.h"
#include "TextPL.h"
#include "ParticleStructures.h"

#include "TileWorld.h"

#include <stdint.h>
#include <string>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Vertex2D.h"
#include "GraphicsTemplate.h"
#include "ComputeTemplate.h"
#include "globalBufferDefinitions.h"
#include "ShaderTypes.h"
#include "ShaderUtility.h"

// ---------------- Shapes

class ColoredQuadPL {
public:

	ColoredQuadPL(VKEngine* engine, int maxInstances)
		: pipeline(engine), engine(engine), maxInstances(maxInstances) { }

	void CreateGraphicsPipeline(const PipelineParameters& params);
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount);

	ShaderTypes::ColoredQuadInstance* getUploadMappedBuffer() {
		return instanceDataDB.buffersMapped[engine->currentFrame]->instanceData;
	}

	const int maxInstances;

private:
	MappedDoubleBuffer<ShaderTypes::ColoredQuadInstaceBuffer> instanceDataDB;
	GraphicsTemplate pipeline;
	VKEngine* engine;
};



class ColoredTrianglesPL {
public:

	ColoredTrianglesPL(VKEngine* engine, int maxTriangles) : pipeline(engine), engine(engine), maxTriangles(maxTriangles) {
	}

	void CreateGraphicsPipeline(PipelineParameters& params);
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int triangleCount);

	Vertex2D* GetVertexMappedBuffer(int frame) {
		return vertexDB.buffersMapped[frame];
	}
	ShaderTypes::ColoredTriangleInstance* GetColorMappedBuffer(int frame) {
		return instanceDB.buffersMapped[frame]->instanceData;
	}

	const static int verticesPerMesh = 3;

	const int maxTriangles;
private:
	GraphicsTemplate pipeline;

	MappedDoubleBuffer<Vertex2D> vertexDB;
	MappedDoubleBuffer<ShaderTypes::ColoredTriangleInstaceBuffer> instanceDB;

	VKEngine* engine;
};


class TexturedQuadPL {
public:

	TexturedQuadPL(VKEngine* engine, int maxInstances)
		: pipeline(engine), engine(engine), maxInstances(maxInstances) {
	}

	void CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, bool lightMapEnabled, std::array<int, 2> lightMapTextureIndexes = { 0, 0 });
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int instanceCount);

	ShaderTypes::TexturedQuadInstance* getUploadMappedBuffer() {
		return instanceDB.buffersMapped[engine->currentFrame]->instanceData;
	}

	const int maxInstances;

private:

	VKEngine* engine;
	GraphicsTemplate pipeline;

	GlobalImageDescriptor* textureDescriptor = nullptr;

	MappedDoubleBuffer<ShaderTypes::TexturedQuadInstaceBuffer> instanceDB;
	MappedDoubleBuffer<ShaderTypes::LightMapUBO> lightmapIndexDB;
};

// ---------------- Particles

constexpr int MAX_PARTICLES_SMALL = 400;
//constexpr int MAX_PARTICLE_SYSTEMS_SMALL = 20;
constexpr int MAX_PARTICLES_LARGE = 400000;
constexpr int MAX_PARTICLE_SYSTEMS_LARGE = 4;

// shader parity
static_assert(sizeof(ShaderTypes::ParticleGroup_small) == sizeof(ShaderTypes::Particle) * MAX_PARTICLES_SMALL);
static_assert(sizeof(ShaderTypes::ParticleGroup_large) == sizeof(ShaderTypes::Particle) * MAX_PARTICLES_LARGE);

class ParticleSystemPL {
public:

	ParticleSystemPL(VKEngine* engine, int maxSystemsSmall) :
		pipeline(engine), engine(engine), maxSystemsSmall(maxSystemsSmall) {
	}

	void CreateGraphicsPipeline(const PipelineParameters& params, const DeviceBuffer& deviceParticleDataBuffer);

	// indexes should be within particle size group
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, std::vector<int>& systemIndexes, std::vector<int>& systemSizes, std::vector<int>& systemParticleCounts);

	void UploadInstanceData(ShaderTypes::ParticleGroup_small& psystem, int index) {
		assert(index < maxSystemsSmall);
		particleDB.buffersMapped[engine->currentFrame]->particleGroups_small[index] = psystem;
	}

	const int maxSystemsSmall;

private:

	// cpu driven mapped particle data
	//struct host_particle_ssbo {
		//ParticleGroup_small particleGroups_small[MAX_PARTICLE_SYSTEMS_SMALL];
	//};


	GlobalImageDescriptor* textureDescriptor = nullptr;

	MappedDoubleBuffer<ShaderTypes::ParticalSmallGroupInstanceBuffer> particleDB;

	GraphicsTemplate pipeline;

	VKEngine* engine;
};

class ParticleComputePL {
public:
	/*struct DispatchInfo {
		int systemIndex;
		int particleCount;
		int particlesToSpawn;
		bool init;
		glm::vec2 spawnPosition;
	};*/

	ParticleComputePL(VKEngine* engine) :
		pipeline(engine), engine(engine) {
	}

	void CreateComputePipeline(const std::vector<uint8_t>& compSrc, DeviceBuffer& particleDataBuffer);

	void RecordCommandBuffer(vk::CommandBuffer commandBuffer, float deltaTime, std::vector<ShaderTypes::ParticleDispatchInfo>& dispatchInfo);

	void UploadInstanceConfigurationData(ParticleSystemConfiguration& psystem, int index) {
		assert(index < MAX_PARTICLE_SYSTEMS_LARGE);

		sysConfigDB.buffersMapped[engine->currentFrame][index] = psystem;
	}

private:


	MappedDoubleBuffer<ParticleSystemConfiguration> sysConfigDB;

	ComputeTemplate pipeline;

	VKEngine* engine;

	DeviceBuffer* particleDataBuffer;

	DeviceBuffer atomicCounterBuffer;
};


// ------------ Tilemap stuff

class TilemapPL {
public:

	TilemapPL(VKEngine* engine) : pipeline(engine), engine(engine) {}

	void CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, TileWorldDeviceResources* tileWorldData);
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int textureIndex, int lightMapIndex);

private:

	VKEngine* engine;
	GraphicsTemplate pipeline;
};

class TilemapLightRasterPL {
public:

	TilemapLightRasterPL(VKEngine* engine) : pipeline(engine), engine(engine) {
	}

	void CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor, TileWorldDeviceResources* tileWorldData);
	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int textureIndex, const ShaderTypes::LightingSettings& lightingSettings);

private:
	TileWorldDeviceResources* tileWorldData = nullptr;

	GlobalImageDescriptor* textureDescriptor;
	GraphicsTemplate pipeline;

	VKEngine* engine;
};


class LightingComputePL {

public:

	LightingComputePL(VKEngine* engine) : engine(engine), pipeline_pass1(engine), pipeline_pass2(engine), pipeline_pass3(engine) {
	}

	void CreateComputePipeline(
		const std::vector<uint8_t>& computeSrc_firstPass,
		const std::vector<uint8_t>& computeSrc_secondPass,
		const std::vector<uint8_t>& computeSrc_thirdPass,
		TileWorldDeviceResources* tileWorldData
	);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, int baseUpdates, int blurUpdates, const ShaderTypes::LightingSettings& lightingSettings);

	void stageLightingUpdate(std::vector<ShaderTypes::LightingUpdate>& baseUpdates, std::vector<ShaderTypes::LightingUpdate>& blurUpdates) {
		ZoneScoped;
		if (baseUpdates.size() != 0)
			std::copy(baseUpdates.begin(), baseUpdates.end(), baseLightUpdateDB.buffersMapped[engine->currentFrame]->baseLightingUpdates);

		if (blurUpdates.size() != 0)
			std::copy(blurUpdates.begin(), blurUpdates.end(), blurLightUpdateDB.buffersMapped[engine->currentFrame]->blurLightingUpdates);
	}

private:

	TileWorldDeviceResources* tileWorldData;

	VKEngine* engine;

	ComputeTemplate pipeline_pass1;
	ComputeTemplate pipeline_pass2;
	ComputeTemplate pipeline_pass3;

	MappedDoubleBuffer<ShaderTypes::baseLightingObjectBuffer> baseLightUpdateDB;
	MappedDoubleBuffer<ShaderTypes::blurLightingObjectBuffer> blurLightUpdateDB;
};