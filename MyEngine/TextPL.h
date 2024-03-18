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

	struct textHeader {
		glm::vec4 color;
		alignas(8) glm::vec2 position;
		alignas(8) glm::vec2 scale;
		float rotation;
		int _textureIndex;
		int textLength;

		uint32_t padding[1];
	};
	static_assert(sizeof(textHeader) % 16 == 0);

	TextPL(VKEngine* engine, int maxObjects, int maxStringLength) 
		: pipeline(engine), engine(engine), maxObjects(maxObjects), maxStringLength(maxStringLength) {}

	void CreateGraphicsPipeline(const PipelineParameters& params, GlobalImageDescriptor* textureDescriptor);

	void recordCommandBuffer(vk::CommandBuffer commandBuffer);

	void ClearTextData(int frame) {
		for (size_t i = 0; i < maxObjects; i++)
			textHeadersDB.buffersMapped[frame][i].textLength = 0;
	}

	// TODO replace with function to get pointers to a given memory slot for direct memory access to avoid copy
	void UploadTextData(int frame, int memorySlot, textHeader& header, fontID font, std::vector<charQuad>& quads) {

		// transfers memory to GPU 
		textHeadersDB.buffersMapped[frame][memorySlot] = header;
		std::copy(quads.begin(), quads.end(), textQuadsDB.buffersMapped[frame] + memorySlot * maxStringLength);
	};

	const int maxObjects;
	const int maxStringLength;

private:

	struct alignas(16) letterIndexInfo {
		uint32_t headerIndex;
		uint32_t letterIndex;
	};

	GlobalImageDescriptor* textureDescriptor = nullptr;

	VKEngine* engine;

	GraphicsTemplate pipeline;

	MappedDoubleBuffer<textHeader> textHeadersDB;
	MappedDoubleBuffer<charQuad> textQuadsDB;
	MappedDoubleBuffer<letterIndexInfo> letterIndexDB;
};
