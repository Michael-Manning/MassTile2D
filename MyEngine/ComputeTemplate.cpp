#include "stdafx.h"

#include <memory>
#include <stdint.h>
#include <vector>

#include <glm/glm.hpp>

#include "descriptorManager.h"
#include "ComputeTemplate.h"

using namespace glm;
using namespace std;

void ComputeTemplate::CreateComputePipeline(const ShaderResourceConfig& resourceConfig) {

	assert(resourceConfig.computeSrcStages.size() > 0);

	auto computeStages = createComputeShaderStages(resourceConfig.computeSrcStages);

	this->pushInfo = resourceConfig.pushInfo;

	descriptorManager.configureDescriptorSets(resourceConfig.descriptorInfos);
	descriptorManager.buildDescriptorLayouts();


	descriptorLayoutMap setLayouts;

	for (auto& [set, layout] : descriptorManager.builderLayouts)
		setLayouts[set] = layout;

	for (auto& globalDesc : resourceConfig.globalDescriptors)
		setLayouts[globalDesc.setNumber] = globalDesc.descriptor->layout;


	buildPipelineLayout(setLayouts, pushInfo.pushConstantSize, pushInfo.pushConstantShaderStages);

	vk::ComputePipelineCreateInfo pipelineInfo;
	pipelineInfo.layout = pipelineLayout;

	compPipelines.reserve(computeStages.size());
	for (auto& stage : computeStages)
	{
		pipelineInfo.stage = stage;
		auto ret = engine->devContext.device.createComputePipeline({}, pipelineInfo);
		assert(ret.result == vk::Result::eSuccess);
		compPipelines.push_back(ret.value);
	}

	descriptorManager.buildDescriptorSets();

	// need to save for binding in command buffer
	globalDescriptors = resourceConfig.globalDescriptors;
}


void ComputeTemplate::BindPipelineStage(vk::CommandBuffer& commandBuffer, int index){
	assert(index >= 0 && index < compPipelines.size());
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, compPipelines[index]);
}

void ComputeTemplate::BindDescriptorSets(vk::CommandBuffer& commandBuffer) {	

	// handle potential global descriptor
	for (auto& desc : globalDescriptors)
	{
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, desc.setNumber, 1, &desc.descriptor->descriptorSets[engine->currentFrame], 0, VK_NULL_HANDLE);
	}

	for (auto& detail : descriptorManager.builderDescriptorSetsDetails) {
		commandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eCompute,
			pipelineLayout,
			detail.set,
			1,
			reinterpret_cast<vk::DescriptorSet*>(&descriptorManager.builderDescriptorSets[detail.set][engine->currentFrame]),
			0,
			nullptr);
	}
}


void ComputeTemplate::UpdatePushConstant(vk::CommandBuffer& commandBuffer, void* pushConstantData) {
	assert(pushInfo.pushConstantSize > 0);
	commandBuffer.pushConstants(pipelineLayout, pushInfo.pushConstantShaderStages, 0, pushInfo.pushConstantSize, pushConstantData);
}

/*
	How to dispatch a compute shader and understand thread group sizes vs multidimensional group counts:

	The group size is defined in the shader layout,
	The group counts are specified by the dispatch command.

	If the layout of a shader is something like (32, 32, 1):
	-layout (local_size_x = 32, local_size_y = 32, local_size_z = 1)

	and the dispatch call is (10, 1, 1):
	-commandBuffer.dispatch(10, 1, 1);

	That will create 10 work groups that have 1024 threads each which are laid out as 32 by 32, with 10,240 total threads.
	The range of gl_LocalInvocationID will be 0-31 on both x and y axes, and 0-0 on the z axis. 
	gl_WorkGroupID will be from 0-9 on the x axis, and 0-0 on the x and y axes. 
	gl_GlobalInvocationID will range from 0-319 on the x axis, 0-31 on the y axis, and 0-0 on the z axis.

	A dispatch call of (10, 10, 1) with the same layout will create 100 groups of 1024 threads with 102,400 total threads.
	The range of gl_LocalInvocationID will still be 0-31 on both x and y axes and 0-0 on the z axis.
	gl_WorkGroupID will range from 0-9 on the x axis, 0-9 the y axis, and 0-0 on the z axis.
	gl_GlobalInvocationID will range from 0-319 on the x axis, 0-319 on the y axis, and 0-0 on the z axis.

	The groupCounts and axes in the dispatch call have no relation to the local size or gl_LocalInvocationID of the shader,
	but they do affact the range of gl_WorkGroupID and gl_GlobalInvocationID within the shader.
	The parameters of the dispatch command create a 3-dimensional grid of groups, each the size of the local_size defined in the shader.
	So, the total number of threads = local_size_x * local_size_y * local_size_z * groupCountX * groupCountY * groupCountZ.
	
	The only way the distribution of thread groups along different axes in the dispatch command affects shader code (e.g (10, 10, 1) vs (100, 1, 1),
	is the range of values observed in gl_WorkGroupID.xyz and gl_GlobalInvocationID.xyz. The range of values for gl_LocalInvocationID.xyz
	is determined only by the local_size in the shader layout and nothing else. The purpose of distributing work groups across
	multiple axes instead of one is mainly for working with a large grid of data like an image to utilise the two dimensional range of
	gl_GlobalInvocationID.xy. A single work group dimension might make more sense for a one-dimensional data set, or a one
	dimensional array of a structure contains 1-3 dimensional data (using a multidimensional local size). 
	There are also performance considerations for choosing different local sizes and different work group distributions across multiple dimensions, 
	but it's recommended to choose what best fits that data being worked on.

	Generally, there are two ways the thread groups are distributed.

	1: For operating on one large grid (in this case a 2D image), a local work group size is defined in the shader somewhat arbitrarily,
	but limited by the maxComputeWorkGroupSize[3] defined by VkPhysicalDeviceProperties. I have found no good explanation of how to 
	determine the optimal size other than "it depends", but generally, larger is better and 32 by 32 is usually the upper limit.
	The dispatch command will have a >1 thread count in both X and Y axes. The values should be the image width / work group size x
	and the image height / work group size y. If these are not perfectly divisible, +1 must be added to both dimensions which will create
	some unnecessary threads assuming you want one thread per pixel, so that must be accounted for in the shader.
	So essentially, the command would be dispatch(imageWidth / local_size_x + (imageWidth % local_size_x ? 1 : 0, ,); with the groupCountY
	following the same logic, and the groupCountZ being 1. In the shader, the gl_GlobalInvocationID.xy is used to determine what pixel each
	thread is working on, and a check for returning early if the x and y invocation IDs are out of range is needed if the image dimensions are not
	a value divisible by the local_size. The gl_LocalInvocationID and gl_WorkGroupID are not used at all here because we don't actually care
	about the work group size or what work group we're in. The work group size is just something we're required to choose in order to 
	get the command to work. We only care about the gl_GlobalInvocationID because our data is one large set not broken up in any meaningful way.

	2: For working on a single dimensional array of data or a list of multi dimensional structures of data, a local work group size is chosen
	to fit the data. If the data is a one-dimensional primitive array, you would usually just choose the largest sensible local_size_x, dispatch 
	the groups with only the x axis being >1, and use the same technique as the grid for indexing with gl_GlobalInvocationID.x. 
	If the data is more structured, such as chunks of data which represent jobs working on 32 by 32 grids of data, the gl_GlobalInvocationID 
	might be used for indexing within a buffer of jobs. The gl_LocalInvocationID.xy would then be used for identifying which piece of data to 
	operate on within the 2D data structure for the job. 
	If the job data structure is too large for the maximum local group size, more creative arithmetic or an additional dimensions will be needed.

	Generally,either the gl_GlobalInvocationID is used alone for large contiguous data, or a combination of gl_WorkGroupID and gl_LocalInvocationID
	is used to fit the shape of the data being operated on.
*/

void ComputeTemplate::DispatchGrid(vk::CommandBuffer& commandBuffer, glm::ivec3 gridSize, glm::ivec3 local_size) {
	commandBuffer.dispatch(
		gridSize.x / local_size.x + (gridSize.x % local_size.x ? 1 : 0),
		gridSize.y / local_size.y + (gridSize.y % local_size.y ? 1 : 0),
		gridSize.z / local_size.z + (gridSize.z % local_size.z ? 1 : 0)
	);
}

void ComputeTemplate::DispatchGroups(vk::CommandBuffer& commandBuffer, glm::ivec3 workGroupCounts) {
	commandBuffer.dispatch(workGroupCounts.x, workGroupCounts.y, workGroupCounts.z);
}