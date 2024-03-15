#pragma once

#include <vector>
#include <stdint.h>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "ComputeTemplate.h"
#include "ParticleSystemPL.h"


class ParticleComputePL {
public:

	struct DispatchInfo{
		int systemIndex;
		int particleCount;
		int particlesToSpawn;
		bool init;
		glm::vec2 spawnPosition;
	};

	ParticleComputePL(VKEngine* engine) :
		pipeline(engine), engine(engine) { }

	void CreateComputePipeline(const std::vector<uint8_t>& compSrc, DeviceBuffer& particleDataBuffer);

	void RecordCommandBuffer(vk::CommandBuffer commandBuffer, float deltaTime, std::vector<DispatchInfo>& dispatchInfo);



	//struct testStr
	//{
	//	int32_t particleCount = 200;
	//	uint32_t burstMode = 0;
	//	uint32_t burstRepeat = 1;
	//	float spawnRate = 0;
	//	float particleLifeSpan = 0;
	//	float gravity = 0;
	//	float startSize = 0;
	//	float endSize = 0;
	//	alignas(16) float startColor[4] = { 0, 0, 0, 0 };
	//	alignas(16) float endColor[4] = { 0, 0, 0, 23 };
	//};

	//struct ParticleSystemConfiguration2 {
	//	int32_t particleCount = 200;
	//	bool burstMode = false;
	//	bool burstRepeat = false;
	//	float spawnRate = 100.0f; 
	//	float particleLifeSpan = 1.0f; 
	//	float gravity = -10.0f;
	//	float startSize = 0.3;
	//	float endSize = 0.0;
	//	alignas(16) glm::vec4 startColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	//	alignas(16) glm::vec4 endColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	//};

	void UploadInstanceConfigurationData(ParticleSystemPL::ParticleSystemConfiguration& psystem, int index) {
		assert(index < MAX_PARTICLE_SYSTEMS_LARGE);

		//auto tt = sizeof(bool);

		//testStr tst;
		////ParticleSystemPL::ParticleSystemConfiguration tst;

		//tst.particleCount = psystem.particleCount;
		//tst.burstMode = psystem.burstMode;
		//tst.burstRepeat = psystem.burstRepeat;
		//tst.spawnRate = psystem.spawnRate;
		//tst.particleLifeSpan = psystem.particleLifeSpan;
		//tst.gravity = psystem.gravity;
		//tst.startSize = psystem.startSize;
		//tst.endSize = psystem.endSize;

		//tst.startColor[0] = psystem.startColor.r;
		//tst.startColor[1] = psystem.startColor.g;
		//tst.startColor[2] = psystem.startColor.b;
		//tst.startColor[3] = psystem.startColor.a;
		//tst.endColor[0] = psystem.endColor.r;
		//tst.endColor[1] = psystem.endColor.g;
		//tst.endColor[2] = psystem.endColor.b;
		//tst.endColor[3] = psystem.endColor.a;

		//uint8_t* tstptr = reinterpret_cast<uint8_t*> (&tst);

		//uint8_t bufff[64];
		//for (size_t i = 0; i < 64; i++)
		//{
		//	bufff[i] = tstptr[i];
		//}



		//memcpy(sysConfigDB.buffersMapped[engine->currentFrame], &bufff, 64);

		sysConfigDB.buffersMapped[engine->currentFrame][index] = psystem;
		//sysConfigDB.buffersMapped[engine->currentFrame]->systemConfigurations[index] = psystem;
	}


private:

	//// mapped configuration buffer
	//struct device_particleConfiguration_ssbo {
	//	ParticleSystemPL::ParticleSystemConfiguration systemConfigurations[MAX_PARTICLE_SYSTEMS_LARGE];
	//};

	//MappedDoubleBuffer<device_particleConfiguration_ssbo> sysConfigDB;
	
	MappedDoubleBuffer<ParticleSystemPL::ParticleSystemConfiguration> sysConfigDB;

	ComputeTemplate pipeline;

	VKEngine* engine;

	DeviceBuffer* particleDataBuffer;

	DeviceBuffer atomicCounterBuffer;
};