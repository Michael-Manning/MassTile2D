#pragma once

#include <vector>

#include <nlohmann/json.hpp>
#include <robin_hood.h>
#include <assetPack/SceneData_generated.h>

#include "ECS.h"
#include "Physics.h"
#include "Behaviour.h"

class SceneData {
public:

	robin_hood::unordered_node_map<entityID, Entity> entities;
	robin_hood::unordered_node_map<entityID, std::unique_ptr<Behaviour>> behaviours;

	robin_hood::unordered_node_map<entityID, ColorRenderer> colorRenderers;
	robin_hood::unordered_node_map<entityID, SpriteRenderer> spriteRenderers;
	robin_hood::unordered_node_map<entityID, TextRenderer> textRenderers;
	robin_hood::unordered_node_map<entityID, ParticleSystemRenderer> particleSystemRenderers;
	robin_hood::unordered_node_map<entityID, Rigidbody> rigidbodies;
	robin_hood::unordered_node_map<entityID, Staticbody> staticbodies;

	nlohmann::json serializeJson(bool ignorePersistence = false) const;
	static void deserializeJson(nlohmann::json& j, SceneData* sceneData);
	static void deserializeFlatbuffers(const AssetPack::SceneData* s, SceneData* sceneData);

	// inserts of overwrites an entity at the specified ID with blank entity without affecting components
	Entity* EmplaceEntity(entityID ID);

	std::vector<spriteID> getUsedSprites() const;
	std::vector<fontID> getUsedFonts() const;

	IDGenerator<entityID> EntityGenerator;
};