#pragma once


#include <vector>
#include <stdint.h>
#include <memory>
#include <cassert>
#include <unordered_map>
#include <random>

#ifdef _DEBUG
#include <Windows.h>
#endif

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <box2d/box2d.h>

#include <tracy/Tracy.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <vma/vk_mem_alloc.h>

#include "texture.h"
#include "VKEngine.h"

#include "coloredQuadPL.h"
#include "texturedQuadPL.h"
#include "tilemapPL.h"
#include "LightingComputePL.h"
#include "TextPL.h"

#include "Sprite.h"
#include "Font.h"

#include "MyMath.h"
#include "engine.h"
//#include "SpriteRenderer.h"	
#include "ECS.h"
#include "Physics.h"
#include "Entity.h"
#include "Vertex2D.h"
#include "profiling.h"
#include "Settings.h"
#include "GlobalImageDescriptor.h"
#include "AssetManager.h"
#include "ResourceManager.h"
#include "Input.h"
//#include "TextRenderer.h"
#include "IDGenerator.h"
#include "BehaviorRegistry.h"
//#include "GlobalImageDescriptor.h"
#include "Drawlist.h"
