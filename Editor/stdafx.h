#pragma once

#ifndef IM_VEC2_CLASS_EXTRA
#define IM_VEC2_CLASS_EXTRA                                                 \
    ImVec2(const glm::vec2& f) { x = f.x; y = f.y; }                        \
    operator glm::vec2() const { return glm::vec2(x,y); }                   \
    ImVec2& operator+=(const glm::vec2& f) { x += f.x; y += f.y; return *this; } \
    ImVec2& operator-=(const glm::vec2& f) { x -= f.x; y -= f.y; return *this; }
#endif // !IM_VEC2_CLASS_EXTRA


#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Editor.h"
#include "Utils.h"
#include "MyMath.h"
#include "BehaviorRegistry.h"
#include "FileDialog.h"