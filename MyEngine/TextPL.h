#pragma once

#include <vector>
#include <stdint.h>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "VKEngine.h"
#include "pipeline.h"
#include "typedefs.h"
#include "Constants.h"
#include "Font.h"
#include "globalBufferDefinitions.h"
#include "GraphicsTemplate.h"

class TextPL {
public:

	TextPL(VKEngine* engine, int maxObjects, int maxStringLength) 
		: pipeline(engine), engine(engine), maxObjects(maxObjects), maxStringLength(maxStringLength) {}

	void CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer);

	void ClearTextData(int frame) {
		for (size_t i = 0; i < maxObjects; i++)
			textHeadersDB.buffersMapped[frame]->headerData[i].textLength = 0;
	}

	// TODO replace with function to get pointers to a given memory slot for direct memory access to avoid copy
	void UploadTextData(int frame, int memorySlot, const ShaderTypes::TextHeader& header, fontID font, std::vector<ShaderTypes::CharQuad>& quads) {

		// transfers memory to GPU 
		textHeadersDB.buffersMapped[frame]->headerData[memorySlot] = header;
		std::copy(quads.begin(), quads.end(), textQuadsDB.buffersMapped[frame]->textData + memorySlot * maxStringLength);
	};

	const int maxObjects;
	const int maxStringLength;

private:

	GlobalImageDescriptor* textureDescriptor = nullptr;

	VKEngine* engine;

	GraphicsTemplate pipeline;

	MappedDoubleBuffer<ShaderTypes::TextHeaderInstaceBuffer> textHeadersDB;
	MappedDoubleBuffer<ShaderTypes::TextDataInstaceBuffer> textQuadsDB;
	MappedDoubleBuffer<ShaderTypes::TextIndexInstaceBuffer> letterIndexDB;
};
