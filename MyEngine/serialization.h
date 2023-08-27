#pragma once


#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <initializer_list>

template <typename T>
static T fromJson(nlohmann::json j);



static auto toJson(glm::vec4& v) {
	return std::initializer_list{v.x, v.y, v.z, v.w};
};
template <>
static glm::vec4 fromJson(nlohmann::json j) {
	return glm::vec4(j[0], j[1], j[2], j[3]);
};

static auto toJson(glm::vec2& v) {
	return std::initializer_list{v.x, v.y};
}
template <>
static glm::vec2 fromJson(nlohmann::json j) {
	return glm::vec2(j[0], j[1]);
}