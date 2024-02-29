#pragma once


#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <initializer_list>

#include <assetPack/common_generated.h>

template <typename T>
static T fromJson(nlohmann::json j);



static auto toJson(const glm::vec4& v) {
	return std::initializer_list{v.x, v.y, v.z, v.w};
};
template <>
static glm::vec4 fromJson(nlohmann::json j) {
	return glm::vec4(j[0], j[1], j[2], j[3]);
};

static auto toJson(const glm::vec2& v) {
	return std::initializer_list{v.x, v.y};
}
template <>
static glm::vec2 fromJson(nlohmann::json j) {
	return glm::vec2(j[0], j[1]);
}


#include <flatbuffers/flatbuffers.h>
#include <assetPack/common_generated.h>

inline glm::ivec2 fromAP(const AssetPack::ivec2* v) {
	return glm::ivec2(v->x(), v->y());
};

inline glm::ivec2 fromAP(const AssetPack::ivec2 v) {
	return glm::ivec2(v.x(), v.y());
};

inline glm::vec2 fromAP(const AssetPack::vec2* v) {
	return glm::vec2(v->x(), v->y());
};

inline glm::vec2 fromAP(const AssetPack::vec2 v) {
	return glm::vec2(v.x(), v.y());
};

inline glm::vec4 fromAP(const AssetPack::vec4* v) {
	return glm::vec4(v->r(), v->g(), v->b(), v->a());
};

inline glm::vec4 fromAP(const AssetPack::vec4 v) {
	return glm::vec4(v.r(), v.g(), v.b(), v.a());
};