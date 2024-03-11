#include "stdafx.h"

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
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include"engine.h"
#include "Physics.h"

#include "Editor.h"
#include "Utils.h"
#include "MyMath.h"
#include "BehaviorRegistry.h"
#include "FileDialog.h"

#include "SpriteGenerator.h"
#include "FontGenerator.h"
#include "PrefabGenerator.h"

using namespace ImGui;
using namespace std;
using namespace glm;

float objectMatrix[16];
float viewMatrix[16];
float projectionMatrix[16];	

namespace {

	bool InputFloatGetSetClamp(const char* label,
		std::function<float()> getter,
		std::function<void(float)> setter,
		float minVal,
		float maxVal)
	{
		float value = getter();
		SetNextItemWidth(60.0f);
		if (ImGui::InputFloat(label, &value)) {
			if (value < minVal) {
				value = minVal;
			}
			if (value > maxVal) {
				value = maxVal;
			}
			setter(value);
			return true;
		}
		return false;
	}

	static bool InputFloatClamp(const char* label, float* value, float min, float max) {
		bool b = InputFloat(label, value);
		*value = glm::clamp(*value, min, max);
		return b;
	}

	static bool InputFloatClamp(const char* label, float* value, float min) {
		bool b = InputFloat(label, value);
		*value = glm::max(*value, min);
		return b;
	}

	static bool InputIntClamp(const char* label, int* value, int min, int max) {
		bool b = InputInt(label, value);
		*value = glm::clamp(*value, min, max);
		return b;
	}

	static bool InputIntClamp(const char* label, int* value, int min) {
		bool b = InputInt(label, value);
		*value = glm::max(*value, min);
		return b;
	}

	float GetWorldGridSize(float zoomLevel) {
		if (zoomLevel < 0.02)
			return 10;
		if (zoomLevel < 0.1)
			return 1.0;
		if (zoomLevel < 0.3)
			return 1.0f / 2.0f;
		if (zoomLevel < 0.6)
			return 1.0f / 4.0f;
		else
			return 1.0f / 10.0f;
	}

	float getWorldGridRounding(float value, float zoomLevel) {
		float gridSize = GetWorldGridSize(zoomLevel);
		return roundf(value / gridSize) * gridSize;
	}

	//bool Combo(const char* label, int* current_item, const std::vector<std::string>& items, , int items_count, int height_in_items = -1)
	//{
	//	return Combo(label, current_item, [](void* data, int idx, const char** out_text) { *out_text = ((const std::vector<std::string>*)data)[idx].c_str(); return true; }, (void*)&items, items_count, height_in_items);
	//}

	const float leftPanelWindowWidth = 220;
	const float inspectorWindowWidth = 300;
}

vec2 Editor::DrawSprite(spriteID id, glm::vec2 maxSize) {
	const auto& sprite = engine->assetManager->GetSprite(id);
	const auto& imTexture = engine->assetManager->getSpriteImTextureID(sprite->ID);
	assert(imTexture.has_value());

	if (imTexture.has_value() == false) {
		Text("Could not display texture in Editor");
		return ImGui::GetItemRectSize();
	}

	vec2 imSize = sprite->resolution;
	float iaspect = imSize.x / imSize.y;
	float daspect = maxSize.x / maxSize.y;

	vec2 dispSize;

	if (iaspect > daspect) {
		dispSize.x = maxSize.x;
		dispSize.y = maxSize.x / iaspect;
	}
	else {
		dispSize.y = maxSize.y;
		dispSize.x = maxSize.y * iaspect;
	}

	Image((ImTextureID)imTexture.value(), dispSize);
	return dispSize;
}
vec2 Editor::DrawSpriteAtlas(spriteID id, glm::vec2 maxSize, int atlasIndex) {
	const auto& sprite = engine->assetManager->GetSprite(id);
	const auto& imTexture = engine->assetManager->getSpriteImTextureID(sprite->ID);

	if (imTexture.has_value() == false) {
		Text("Could not display texture in Editor");
		return ImGui::GetItemRectSize();
	}

	if (sprite->atlas.size() > 0) {
		auto atlasEntry = sprite->atlas[atlasIndex];

		vec2 uvSize = atlasEntry.uv_max - atlasEntry.uv_min;
		float iaspect = uvSize.x / uvSize.y;
		float daspect = maxSize.x / maxSize.y;

		vec2 dispSize;

		if (iaspect > daspect) {
			dispSize.x = maxSize.x;
			dispSize.y = maxSize.x / iaspect;
		}
		else {
			dispSize.y = maxSize.y;
			dispSize.x = maxSize.y * iaspect;
		}

		Text(atlasEntry.name.c_str());
		Image((ImTextureID)imTexture.value(), dispSize, atlasEntry.uv_min, atlasEntry.uv_max);
		return dispSize;
	}
}



void Editor::DrawGameSceneGrid(ImDrawList* drawlist, glm::vec2 size, glm::vec2 offset) {

	float gridSize = GetWorldGridSize(editorCamera.zoom);
	const int gridOpacity = (int)(0.2 * 255);

	{

		float x0 = gameSceneSreenToWorldPos(vec2(offset.x, 0)).x;
		x0 = getWorldGridRounding(x0, editorCamera.zoom);
		x0 = gameSceneWorldToScreenPos(vec2(x0, 0)).x;

		float inc = (gridSize * (size.y / 2.0f)) * editorCamera.zoom;

		float xi = x0;
		while (xi <= offset.x + size.x) {
			drawlist->AddLine(
				vec2(xi, offset.y),
				vec2(xi, offset.y + size.y),
				IM_COL32(255, 255, 255, gridOpacity));
			xi += inc;
		}
	}
	{
		float y0 = gameSceneSreenToWorldPos(vec2(0, offset.y)).y;
		y0 = getWorldGridRounding(y0, editorCamera.zoom);
		y0 = gameSceneWorldToScreenPos(vec2(0, y0)).y;

		float inc = (gridSize * (size.y / 2.0f)) * editorCamera.zoom;

		float yi = y0;
		while (yi <= offset.y + size.y) {
			drawlist->AddLine(
				vec2(offset.x, yi),
				vec2(offset.x + size.x, yi),
				IM_COL32(255, 255, 255, gridOpacity));
			yi += inc;
		}
	}
}

void Editor::DrawPreviewSceneGrid(ImDrawList* drawlist, glm::vec2 size, glm::vec2 offset) {
	float gridSize = GetWorldGridSize(previewCamera.zoom);
	const int gridOpacity = (int)(0.2 * 255);

	{

		float x0 = previewSceneSreenToWorldPos(vec2(offset.x, 0)).x;
		x0 = getWorldGridRounding(x0, previewCamera.zoom);
		x0 = previewSceneWorldToScreenPos(vec2(x0, 0)).x;

		float inc = (gridSize * (size.y / 2.0f)) * previewCamera.zoom;

		float xi = x0;
		while (xi <= offset.x + size.x) {
			drawlist->AddLine(
				vec2(xi, offset.y),
				vec2(xi, offset.y + size.y),
				IM_COL32(255, 255, 255, gridOpacity));
			xi += inc;
		}
	}
	{
		float y0 = previewSceneSreenToWorldPos(vec2(0, offset.y)).y;
		y0 = getWorldGridRounding(y0, previewCamera.zoom);
		y0 = previewSceneWorldToScreenPos(vec2(0, y0)).y;

		float inc = (gridSize * (size.y / 2.0f)) * previewCamera.zoom;

		float yi = y0;
		while (yi <= offset.y + size.y) {
			drawlist->AddLine(
				vec2(offset.x, yi),
				vec2(offset.x + size.x, yi),
				IM_COL32(255, 255, 255, gridOpacity));
			yi += inc;
		}
	}
}

void Editor::EntityGizmo(ImDrawList* drawlist, Camera& camera)
{
	vec2 mpos = input->getMousePos();

	float lineLen = 104; // translate gizmo arm length
	float wheelRad = 140; // rotate gizmo radius

	vec2 objScreenPos = selectedSceneWorldToScreenPos(selectedEntity->GetGlobalTransform().position);
	vec2 yHandlePos = objScreenPos + vec2(0, -lineLen);
	vec2 xHandlePos = objScreenPos + vec2(lineLen, 0);

	if (input->getMouseBtnDown(MouseBtn::Left)) {
		draggingY = glm::distance(mpos, yHandlePos) < 24;
		draggingX = glm::distance(mpos, xHandlePos) < 24;

		// central handle
		if (glm::distance(mpos, objScreenPos) < 24) {
			draggingX = true;
			draggingY = true;
		}

		if ((draggingX || draggingY) == false) {
			float mDist = glm::distance(mpos, objScreenPos);
			if (mDist > wheelRad - 8 && mDist < wheelRad + 8) {
				draggingAngle = true;
				dragInitialAngle = atan2f(mpos.y - objScreenPos.y, mpos.x - objScreenPos.x);
				dragInitialObjectAngle = selectedEntity->transform.rotation;
			}
		}
	}

	{
		bool mDown = input->getMouseBtn(MouseBtn::Left);
		draggingY &= mDown;
		draggingX &= mDown;
		draggingAngle &= mDown & (!draggingX) & (!draggingY);
	}

	if (draggingY) {

		// offset handle length if not grabbing central handle
		float mouseWorldObjPos = selectedSceneSreenToWorldPos(mpos + (draggingX ? 0 : lineLen)).y;

		if (input->getKey(KeyCode::LeftControl))
			yHandlePos.y = roundf(mouseWorldObjPos * 100) / 100; // two decimal places
		else
			mouseWorldObjPos = getWorldGridRounding(mouseWorldObjPos, camera.zoom);

		objScreenPos.y = selectedSceneWorldToScreenPos(vec2(0, mouseWorldObjPos)).y;
		yHandlePos.y = objScreenPos.y - lineLen;
	}

	if (draggingX) {

		float mouseWorldObjPos = selectedSceneSreenToWorldPos(mpos - (draggingY ? 0 : lineLen)).x;

		if (input->getKey(KeyCode::LeftControl))
			xHandlePos.x = roundf(mouseWorldObjPos * 100) / 100; // two decimal places
		else
			mouseWorldObjPos = getWorldGridRounding(mouseWorldObjPos, camera.zoom);

		objScreenPos.x = selectedSceneWorldToScreenPos(vec2(mouseWorldObjPos, 0.0)).x;
		xHandlePos.x = objScreenPos.x + lineLen;

	}

	if (draggingX || draggingY) {
		vec2 objPos = selectedSceneSreenToWorldPos(objScreenPos);
		if (selectedEntity->HasParent()) {
			mat4 m = selectedEntity->GetGlobalToLocalMatrix();
			vec4 tvec(objPos, 0, 1);
			vec4 res = m * tvec;
			objPos = vec2(res);
		}

		selectedEntity->transform.position = objPos;
		if (selectedScene->sceneData.rigidbodies.contains(selectedEntity->ID)) {
			assert(selectedEntity->HasParent() == false);
			auto body = &selectedScene->sceneData.rigidbodies.at(selectedEntity->ID);
			body->SetPosition(objPos);
			body->SetLinearVelocity(vec2(0.0f));
			body->SetAngularVelocity(0.0f);
		}
		if (selectedScene->sceneData.staticbodies.contains(selectedEntity->ID)) {
			assert(selectedEntity->HasParent() == false);
			selectedScene->sceneData.staticbodies.at(selectedEntity->ID).SetPosition(objPos);
		}
	}

	if (draggingAngle) {
		const int rotationSnapAngles = 8;

		float dragAngle = atan2f(mpos.y - objScreenPos.y, mpos.x - objScreenPos.x);
		selectedEntity->transform.rotation = dragInitialObjectAngle + dragInitialAngle - dragAngle;
		if (input->getKey(KeyCode::LeftControl) == false) {
			float segment = 2.0f * PI / rotationSnapAngles;
			selectedEntity->transform.rotation = roundf(selectedEntity->transform.rotation / segment) * segment;
		}
		if (selectedScene->sceneData.rigidbodies.contains(selectedEntity->ID)) {
			auto body = &selectedScene->sceneData.rigidbodies.at(selectedEntity->ID);
			body->SetRotation(selectedEntity->transform.rotation);
			body->SetAngularVelocity(0.0f);

		}
		if (selectedScene->sceneData.staticbodies.contains(selectedEntity->ID)) {
			selectedScene->sceneData.staticbodies.at(selectedEntity->ID).SetRotation(selectedEntity->transform.rotation);
		}
	}

	auto yGizCol = draggingY ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255);
	auto xGizCol = draggingX ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255);
	auto cGizCol = (draggingX && draggingY) ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255);
	auto wGizCol = (draggingAngle) ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255);


	// axis handles
	drawlist->AddLine(objScreenPos, yHandlePos, yGizCol, 1);
	drawlist->AddTriangleFilled(
		yHandlePos,
		yHandlePos + vec2(8, 14),
		yHandlePos + vec2(-8, 14), yGizCol);

	drawlist->AddLine(objScreenPos, xHandlePos, xGizCol, 1);
	drawlist->AddTriangleFilled(
		xHandlePos,
		xHandlePos + vec2(-14, 8),
		xHandlePos + vec2(-14, -8), xGizCol);

	// central handle
	drawlist->AddRectFilled(objScreenPos - 10.0f, objScreenPos + 10.0f, cGizCol);

	// rotation handle
	drawlist->AddCircle(objScreenPos, wheelRad, wGizCol);

}


void Editor::controlWindow() {

	const float margin = 10.0f;
	auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
	SetNextWindowPos(vec2(leftPanelWindowWidth + margin, margin + 30));
	SetNextWindowSize(vec2(engine->getWindowSize().x - leftPanelWindowWidth - inspectorWindowWidth - (margin * 2.0f), 40));
	Begin("control", nullptr, flags);

	if (engine->time - updateTimer > 0.2f) {
		frameRateStat = engine->_getAverageFramerate();
		updateTimer = engine->time;
	}

	if (Button("Show Stats")) {
		showingStats = true;
	}
	if (engine->assetManager->HasScene(selectedScene->name)) {
		SameLine();
		if (Button("Reload Scene")) {
			loadGameSceneFromDisk(selectedScene->name);

		}
	}
	SameLine();
	Text("%.2f", frameRateStat);
	SameLine();
	if (gameScene->paused) {
		Text("paused");
	}
	else {
		Text("playing");
	}
	SameLine();
	Text("zoom: %.2f", editorCamera.zoom);
	SameLine();
	Text("Camera: %.2f, %.2f", editorCamera.position.x, editorCamera.position.y);

	End();
}

void Editor::EntitySelectableTree(int& index, Entity* entity, Scene* scene) {

	if (Selectable((entity->name + "##" + to_string(index)).c_str(), entity == selectedEntity)) {
		clearInspectorSelection();

		// already selected. deselect
		if (selectedEntity == entity) {
			selectedEntity = nullptr;
		}
		// select new
		else {
			selectedEntity = entity;
		}
	}

	if (ImGui::BeginPopupContextItem()) {
		index++;
		if (ImGui::MenuItem("Create child")) {
			auto child = scene->CreateEntity({}, "", true);
			scene->SetEntityAsChild(entity, child);
			child->name = selectedScene->GetNoneConflictingEntityName(child, entity);
			selectedEntity = nullptr;
			ImGui::EndPopup();
			return;
		}
		if (ImGui::MenuItem("Delete")) {
			scene->DeleteEntity(entity->ID, true);
			selectedEntity = nullptr;
			ImGui::EndPopup();
			return;
		}
		if (ImGui::MenuItem("Duplicate")) {
			auto newEntity = scene->DuplicateEntity(entity);
			if (entity->HasParent()) {
				selectedScene->SetEntityAsChild(entity->GetParent(), newEntity->ID);
				newEntity->name = selectedScene->GetNoneConflictingEntityName(newEntity, selectedScene->GetEntity(entity->GetParent()));
			}
			else {
				newEntity->name = selectedScene->GetNoneConflictingEntityName(newEntity, nullptr);
			}
			selectedEntity = nullptr;
			ImGui::EndPopup();
			return;
		}
		if (ImGui::MenuItem("Save as prefab")) {
			selectedEntity = nullptr;
			ImGui::EndPopup();

			//char filename[MAX_PATH];
			//if (saveFileDialog(filename, MAX_PATH, "prefab", "Prefab File\0*.prefab\0")) {
			Prefab p;
			GeneratePrefab(entity, scene->sceneData, &p);

			std::filesystem::path dir(engine->assetManager->directories.prefabDir);
			std::filesystem::path fullPath = dir / std::filesystem::path(p.name);

			engine->assetManager->ExportPrefab(p, fullPath.string());

			engine->assetManager->LoadPrefab(p.name, false);
			return;
		}
		ImGui::EndPopup();
	}
	else {
		index++;
	}

	if (entity->ChildCount() > 0)
	{
		if (TreeNode("children")) {
			auto children = entity->_GetChildCache_ptr();
			for (auto& child : *children)
			{
				EntitySelectableTree(index, child, scene);
			}
			ImGui::TreePop();
		}
	}
}

void Editor::entityWindow() {

	auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse;
	SetNextWindowPos(vec2(0.0f, 0.0f));
	SetNextWindowSize(vec2(leftPanelWindowWidth, engine->getWindowSize().y / 2));

	// cannot iterate the map as it could be modified in the loop
	vector<Entity*> entityList;
	entityList.reserve(selectedScene->sceneData.entities.size());
	for (auto& [entID, entity] : selectedScene->sceneData.entities)
		entityList.push_back(&entity);

	if (showingPreviewWindow) {
		Begin("Prefab", nullptr, flags);

		if (Button("Close")) {
			closePreviewWindow();
		}
		SameLine();
		if (Button("Overwrite")) {
			Prefab p;
			GeneratePrefab(topLevelPrefabPreview, entityPreviewScene->sceneData, &p);

			std::filesystem::path dir(engine->assetManager->directories.prefabDir);
			std::filesystem::path fullPath = dir / std::filesystem::path(p.name);

			engine->assetManager->ExportPrefab(p, fullPath.string());

			engine->assetManager->LoadPrefab(p.name, false);
		}

		int i = 0;
		for (auto& entity : entityList)
		{
			// start with top level entities
			if (!entity->HasParent()) {
				EntitySelectableTree(i, entity, entityPreviewScene.get());
			}
		}
		End();
	}
	else {
		Begin("Entities", nullptr, flags);

		if (Selectable(gameScene->name.c_str(), sceneSelected)) {
			clearInspectorSelection();
			sceneSelected = true;
		}

		if (Button("new")) {	
			auto newEntity = gameScene->CreateEntity({gameSceneSreenToWorldPos(sceneWinPos + sceneWinSize / 2.0f), vec2(1.0f), 0.0f}, "", true);
			newEntity->name = selectedScene->GetNoneConflictingEntityName(newEntity, nullptr);
			selectedEntity = newEntity;
		}

		int i = 0;
		for (auto& entity : entityList)
		{
			// start with top level entities
			if (!entity->HasParent()) {
				EntitySelectableTree(i, entity, gameScene.get());
			}
		}
		End();
	}
}

void Editor::assetWindow() {
	auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse;
	SetNextWindowPos(vec2(0.0f, engine->getWindowSize().y / 2.0f));
	SetNextWindowSize(vec2(leftPanelWindowWidth, engine->getWindowSize().y / 2));
	Begin("Assets", nullptr, flags);

	if (BeginTabBar("assetCategories")) {

		if (BeginTabItem("Scenes")) {
			int i = 0;
			for (auto& p : engine->assetManager->_getLoadedAndUnloadedSceneNames())
			{
				if (Selectable(p.c_str(), selectedSceneIndex == i)) {
					selectedSceneIndex = i;
				}

				if (ImGui::BeginPopupContextItem()) {
					selectedSceneIndex = i;
					i++;
					if (ImGui::MenuItem("Load")) {
						loadGameSceneFromDisk(p);
						selectedSceneIndex = -1;
						ImGui::EndPopup();
						break;
					}
					if (ImGui::MenuItem("Delete from disk")) {
						engine->assetManager->deleteSceneFromDisk(p);
						selectedSceneIndex = -1;
						ImGui::EndPopup();
						break;
					}
					ImGui::EndPopup();
				}
				else {
					i++;
				}
			}
			EndTabItem();
		}

		if (BeginTabItem("Prefabs")) {
			int i = 0;
			for (auto& [name, prefab] :engine->assetManager->_getPrefabIterator())
			{
				if (Selectable(name.c_str(), selectedPrefabIndex == i)) {
					OpenPreviewWindowWithPrefab(prefab);
					selectedPrefabIndex = i;
				}
				else {
					if (IsItemHovered() && input->getMouseBtnDown(MouseBtn::Left)) {
						prefabDragInStarted = true;
						dragPrefab = &prefab;
					}

				}

				if (ImGui::BeginPopupContextItem()) {
					selectedPrefabIndex = i;
					i++;
					if (ImGui::MenuItem("Instantiate")) {
						auto newEntity = gameScene->Instantiate(&prefab, name);
						newEntity->name = gameScene->GetNoneConflictingEntityName(newEntity, nullptr);
						selectedPrefabIndex = -1;
						closePreviewWindow();
						ImGui::EndPopup();
						break;
					}
					if (ImGui::MenuItem("Delete from disk")) {
						
						engine->assetManager->deletePrefabFromDisk(name);

						selectedPrefabIndex = -1;
						closePreviewWindow();
						ImGui::EndPopup();
						break;
					}
					ImGui::EndPopup();
				}
				else {
					i++;
				}
			}
			EndTabItem();
		}

		if (BeginTabItem("Sprites")) {

			if (Button("Create")) {

				char filename[MAX_PATH];
				if (openFileDialog(filename, MAX_PATH, "image", "image file\0*.png\0")) {

					std::string name = getFileRawName(string(filename));
					auto unidentified_sprite = generateSprite_unidentified(name, FilterMode::Linear);
					spriteID sprID = engine->assetManager->ExportSprite(engine->assetManager->directories.assetDir + name, string(filename), unidentified_sprite);
					engine->assetManager->LoadSprite(sprID);
				}
			}

			int i = 0;
			for (auto& p : engine->assetManager->_getSpriteIterator()) {

				if (p.first == engine->assetManager->defaultSpriteID) {
					if (Selectable("default sprite", selectedSpriteIndex == i)) {
						selectedSprite = &p.second;
						selectedSpriteIndex = i;
						selectedSpriteAtlasIndex = 0;
						selectedEntity = nullptr;
					}
				}
				else {
					size_t lastindex = p.second.imageFileName.find_last_of(".");
					std::string rawname = p.second.imageFileName.substr(0, lastindex);
					if (Selectable(rawname.c_str(), selectedSpriteIndex == i)) {
						selectedSprite = &p.second;
						selectedSpriteIndex = i;
						selectedSpriteAtlasIndex = 0;
						selectedEntity = nullptr;
					}
				}
				i++;
			}
			EndTabItem();
		}
		if (BeginTabItem("Fonts")) {

			static vector<string> availFontPaths;
			static char availFontsString[256];

			if (Button("Create")) {
				fontModel = true;
				OpenPopup("Generate Font");

				std::fill(availFontsString, availFontsString + 256, '\0');

				vector<string> paths = getAllFilesInDirectory(engine->assetManager->directories.fontsDir);
				int offset = 0;
				for (auto& s : paths) {
					availFontPaths.push_back(s);
					string fontFileName = std::filesystem::path(s).filename().string();
					strcpy(availFontsString + offset, fontFileName.c_str());
					offset += fontFileName.size() + 1;
				}
			}

			auto modelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
			float modelWidth = 360;
			if (fontModel) {
				SetNextWindowPos(vec2(glm::clamp(engine->winW / 2 - (int)modelWidth / 2, 0, engine->winW), 50));
				SetNextWindowSize(vec2(modelWidth, 300));
			}
			if (BeginPopupModal("Generate Font", &fontModel, modelFlags)) {

				Combo("Font file", &fontComboSelected, availFontsString);

				static FontConfig fontConfig;
				InputInt("First char", &fontConfig.firstChar);
				InputInt("Char count", &fontConfig.charCount);
				InputFloat("Letter height", &fontConfig.fontHeight);
				InputInt2("Atlas size", &fontConfig.atlasWidth);
				InputInt("Oversample", &fontConfig.oversample);
				fontConfig.oversample = glm::clamp(fontConfig.oversample, 1, 8);

				static char fontName[256];
				InputText("Font name", fontName, 256);

				if (strlen(fontName) > 0 && fontComboSelected != -1) {
					if (Button("Generate")) {

						std::string name = string(fontName);
						std::string atlasImageLocation = engine->assetManager->directories.assetDir + name + ".png";
						auto unidentified_font = GenerateFont_unidentified(availFontPaths[fontComboSelected], atlasImageLocation, fontConfig);
						auto unidentified_sprite = generateSprite_unidentified(name, FilterMode::Linear);
						auto fntID = engine->assetManager->ExportFont(engine->assetManager->directories.assetDir + name, engine->assetManager->directories.assetDir + name, atlasImageLocation, unidentified_font, unidentified_sprite);

						engine->assetManager->LoadFont(fntID);

						fontModel = false;
					}
				}

				EndPopup();
			}

			int i = 0;
			for (auto& f : engine->assetManager->_getFontIterator()) {
				Selectable(f.second.name.c_str(), false);
			}

			EndTabItem();
		}
		EndTabBar();
	}
	End();
}

void Editor::debugDataWindow() {
	/*auto flags = ImGuiWindowFlags_NoResize;
	SetNextWindowPos(vec2(0.0f, engine->getWindowSize().y / 2.0f));
	SetNextWindowSize(vec2(entityWindowWidth, engine->getWindowSize().y / 2));*/
	//Begin("Stats", nullptr, flags);

	if (showingStats && Begin("Stats", &showingStats)) {
		auto& stats = engine->_getDebugStats();

		Text("Entity count: %d", stats.entity_count);
		Text("Rendered sprited: %d", stats.sprite_render_count);

		End();
	}
}

void Editor::Initialize(Engine* engine, std::shared_ptr<Scene> gameScene, sceneRenderContextID sceneRenderContext, std::function<void(std::shared_ptr<Scene>)> onMainSceneLoad) {
	this->engine = engine;
	this->gameScene = gameScene;
	selectedScene = gameScene.get();
	this->sceneRenderContext = sceneRenderContext;
	this->setSceneCallback = onMainSceneLoad;

	entityPreviewScene = Scene::MakeScene(engine->assetManager.get()); //make_shared<Scene>(engine->assetManager.get());
	entityPreviewScene->name = "entity preview scene";
	entityPrviewFrameSize = vec2(600);
	entityPreviewsSeneRenderContextID = engine->CreateSceneRenderContext(entityPrviewFrameSize, false, glm::vec4(vec3(0.2), 1.0), 1.0f, false);
	//entityPreviewsSeneRenderContextID = engine->CreateSceneRenderContext(entityPrviewFrameSize, false, glm::vec4(vec3(0.2), 1.0), false);
	entityPreviewFramebuffer = engine->GetSceneRenderContextFramebuffer(entityPreviewsSeneRenderContextID);

	//const auto& pSys = entityPreviewScene->CreateEntity({}, "myEntity");
	/*shared_ptr<Entity> teste = make_shared<Entity>("myEntity");
	auto id = entityPreviewScene->RegisterEntity(teste);*/

	//ParticleSystemRenderer psr = ParticleSystemRenderer(ParticleSystemRenderer::ParticleSystemSize::Small);
	//entityPreviewScene->registerComponent(pSys->ID, ParticleSystemRenderer::ParticleSystemSize::Small);

	//ColorRenderer r;
	//r.color = vec4(1.0, 0, 0, 1.0);
	//teste->transform.scale = vec2(3.0);
	//entityPreviewScene->registerComponent(id, r);
};


void Editor::mainSceneWindow() {

	vec2 mpos = input->getMousePos();

	if (cameraEntityFocus == true) {
		float fracComplete = (engine->time - cameraSlepStartTime) / 0.8f;
		editorCamera.position = smoothstep(camSlerpStart, camSlerpEnd, fracComplete);
		if (fracComplete >= 1.0f)
			cameraEntityFocus = false;
	}


	auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;
	sceneWinPos = vec2(leftPanelWindowWidth, 0.0f);
	sceneWinSize = vec2(engine->getWindowSize().x - inspectorWindowWidth - leftPanelWindowWidth, engine->getWindowSize().y);
	SetNextWindowPos(sceneWinPos);
	SetNextWindowSize(sceneWinSize);
	Begin("Game View", nullptr, flags);

	if (showingPreviewWindow && IsWindowFocused()) {
		closePreviewWindow();
	}

	auto windowDrawlist = GetWindowDrawList();

	vec2 avail = GetContentRegionAvail();
	mainSceneViewerScreenLocation = GetCursorScreenPos();

	windowDrawlist->PushClipRect(mainSceneViewerScreenLocation, mainSceneViewerScreenLocation + avail);

	if (mainSceneFrameSizeChanged)
		engine->ResizeSceneRenderContext(sceneRenderContext, avail);

	//auto cursorPos = ImGui::GetCursorPos();
	vec2 sceneDrawScreenPos = GetCursorScreenPos();
	engine->ImGuiFramebufferImage(sceneFramebuffer, ivec2(avail.x, avail.y));
	//SetCursorPos(cursorPos);
	//InvisibleButton("gameInvisibleBtn", (vec2)POVViewWinSize); // only to prevent window dragging when clicked

	bool sceneHovered = ImGui::IsItemHovered();

	// imgui only "sees" hover if window has been clicked and focused
	bool unFocusedHover = within(sceneDrawScreenPos, sceneDrawScreenPos + avail, input->getMousePos());

	if (unFocusedHover && showingPreviewWindow == false) {
		if (prefabDragInStarted) {
			assert(dragPrefab != nullptr);

			auto newEntity = gameScene->Instantiate(dragPrefab, dragPrefab->name, gameSceneSreenToWorldPos(mpos));
			newEntity->name = gameScene->GetNoneConflictingEntityName(newEntity, nullptr);
			selectedPrefabIndex = -1;

			selectedEntity = newEntity;
			draggingX = true;
			draggingY = true;

			prefabDragInStarted = false;
			dragPrefab = nullptr;
		}
	}

	if (sceneHovered && showingPreviewWindow == false) {

		if (input->getMouseBtn(MouseBtn::Right)) {
			editorCamera.position -= ((mouseDelta * vec2(2.0, -2.0)) / engine->getWindowSize().y) / editorCamera.zoom;
			cameraEntityFocus = false;
		}

		// zoom
		{
			float off = input->GetScrollDelta();
			mainSceneRawZoom += off * 0.02f;
			mainSceneRawZoom = glm::clamp(mainSceneRawZoom, 0.0f, 1.0f);
		}
	}
	editorCamera.zoom = exponentialScale(mainSceneRawZoom, 0.01, 15.1);

	if (showGrid) {
		DrawGameSceneGrid(windowDrawlist, mainSceneFrameSize, mainSceneViewerScreenLocation);
	}

	if (selectedEntity != nullptr && showingPreviewWindow == false) {
		EntityGizmo(windowDrawlist, editorCamera);

		if (input->getMouseBtnDown(MouseBtn::Left) && sceneHovered && !draggingX && !draggingY && !draggingAngle) {
			clearInspectorSelection();
		}
	}

	windowDrawlist->PopClipRect();

	End();
}


static void myFunction(ImDrawList* testDrawlist) {
	auto test = vec2(0);
	auto test2 = vec2(10000);
	testDrawlist->AddLine(test, test2, IM_COL32(255, 255, 255, 255), 500);
}

void Editor::EntityPreviewWindow() {
	vec2 mpos = input->getMousePos();

	auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar;

	vec2 winSize = entityPrviewFrameSize + 20.0f;

	vec2 pos = vec2(
		glm::clamp(sceneWinSize.x / 2 - winSize.x / 2, 0.0f, sceneWinSize.x),
		glm::clamp(sceneWinSize.y / 2 - winSize.y / 2, 0.0f, sceneWinSize.y)
	);

	SetNextWindowPos(pos + sceneWinPos);

	SetNextWindowSize(winSize);

	Begin("Entity Preview", nullptr, flags);

	auto windowDrawlist = GetWindowDrawList();


	vec2 avail = GetContentRegionAvail();
	previewSceneViewerScreenLocation = GetCursorScreenPos();

	windowDrawlist->PushClipRect(previewSceneViewerScreenLocation, previewSceneViewerScreenLocation + avail);

	/*if (mainSceneFrameSizeChanged)
		engine->ResizeSceneRenderContext(sceneRenderContext, avail);*/

	previewSceneViewerScreenLocation = GetCursorScreenPos();

	engine->ImGuiFramebufferImage(entityPreviewFramebuffer, entityPrviewFrameSize);

	bool sceneHovered = ImGui::IsItemHovered();

	if (sceneHovered) {

		if (input->getMouseBtn(MouseBtn::Right)) {
			previewCamera.position -= ((mouseDelta * vec2(2.0, -2.0)) / engine->getWindowSize().y) / previewCamera.zoom;
			cameraEntityFocus = false;
		}

		// zoom
		{
			float off = input->GetScrollDelta();
			previewSceneRawZoom += off * 0.02f;
			previewSceneRawZoom = glm::clamp(previewSceneRawZoom, 0.0f, 1.0f);
		}
	}

	previewCamera.zoom = exponentialScale(previewSceneRawZoom, 0.01, 15.1);


	if (showGrid) {
		DrawPreviewSceneGrid(windowDrawlist, entityPrviewFrameSize, previewSceneViewerScreenLocation);
	}

	if (selectedEntity != nullptr) {
		EntityGizmo(windowDrawlist, previewCamera);

		if (input->getMouseBtnDown(MouseBtn::Left) && sceneHovered && !draggingX && !draggingY && !draggingAngle) {
			clearInspectorSelection();
		}
	}

	windowDrawlist->PopClipRect();

	End();
}

void Editor::loadGameSceneFromDisk(std::string name)
{
	//assert(engine->assetManager->HasScene(name));
	engine->assetManager->LoadScene(name);
	clearInspectorSelection();
	setSceneCallback(engine->assetManager->GetScene(name));
	selectedScene = gameScene.get();
}

void Editor::OpenPreviewWindowWithPrefab(Prefab& prefab) {

	selectedScene = entityPreviewScene.get();

	showingPreviewWindow = true;

	previewCamera.position = vec2(0.0f);
	previewCamera.zoom = 1.0f;

	entityPreviewScene->ClearScene();
	entityPreviewScene->paused = false;
	topLevelPrefabPreview = entityPreviewScene->Instantiate(&prefab);

	selectedEntity = nullptr;
}

void Editor::closePreviewWindow() {
	selectedScene = gameScene.get();
	showingPreviewWindow = false;
	clearInspectorSelection();
}

void Editor::Run() {

	input = engine->GetInput();
	vec2 mpos = input->getMousePos();
	screenSize = engine->getWindowSize();

	sceneFramebuffer = engine->GetSceneRenderContextFramebuffer(sceneRenderContext);
	mainSceneFrameSize = engine->GetFramebufferSize(sceneFramebuffer);
	mainSceneFrameSizeChanged = lastMainSceneFrameSize != mainSceneFrameSize;
	lastMainSceneFrameSize = mainSceneFrameSize;

	auto sceneViewDrawlist = GetForegroundDrawList();

	auto io = GetIO();

	if (io.WantTextInput == false) {

		if (input->getKeyDown('g'))
			showGrid = !showGrid;

		if (selectedEntity != nullptr && input->getKeyDown('f')) {
			camSlerpStart = editorCamera.position;
			camSlerpEnd = selectedEntity->transform.position;
			cameraEntityFocus = true;
			cameraSlepStartTime = engine->time;
		}
	}

	mouseDelta = mpos - lastMpos;
	lastMpos = mpos;

	//// panning
	//{
	//	vec2 delta = mpos - lastMpos;
	//	lastMpos = mpos;

	//	if (input->getMouseBtn(MouseBtn::Right)) {
	//		editorCamera.position -= ((delta * vec2(2.0, -2.0)) / engine->getWindowSize().y) / editorCamera.zoom;
	//		cameraEntityFocus = false;
	//	}
	//}

	//if (cameraEntityFocus == true) {
	//	float fracComplete = (engine->time - cameraSlepStartTime) / 0.8f;
	//	editorCamera.position = smoothstep(camSlerpStart, camSlerpEnd, fracComplete);
	//	if (fracComplete >= 1.0f)
	//		cameraEntityFocus = false;
	//}

	entityWindow();

	assetWindow();

	mainSceneWindow();

	if (showingPreviewWindow == false) {
		controlWindow();
	}

	if (showingPreviewWindow)
	{
		EntityPreviewWindow();

		entityPreviewRenderJob.camera = previewCamera;
		entityPreviewRenderJob.scene = entityPreviewScene;
		entityPreviewRenderJob.sceneRenderCtxID = entityPreviewsSeneRenderContextID;

	}

	{
		auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse;
		SetNextWindowPos(vec2(engine->getWindowSize().x - inspectorWindowWidth, 0.0f));
		SetNextWindowSize(vec2(300, engine->getWindowSize().y));
		Begin("Inspector", nullptr, flags);

		if (selectedEntity != nullptr) {

			if (Combo("Create", &comboSelected, "Sprite Renderer\0Color Renderer\0Text Renderer\0Particle System\0Box Staticbody\0Circle Staticbody\0Box Rigidbody\0Circle Rigidbody")) {
				switch (comboSelected)
				{
				case 0:
					if (!selectedScene->sceneData.spriteRenderers.contains(selectedEntity->ID))
						selectedScene->registerComponent(selectedEntity->ID, engine->assetManager->defaultSpriteID);
					break;
				case 1:
					if (!selectedScene->sceneData.colorRenderers.contains(selectedEntity->ID))
						selectedScene->registerComponent(selectedEntity->ID, ColorRenderer(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
					break;
				case 2:
					if (!selectedScene->sceneData.textRenderers.contains(selectedEntity->ID))
						if (engine->assetManager->_fontAssetCount() != 0)
							selectedScene->registerComponent(selectedEntity->ID, TextRenderer(engine->assetManager->_getFontIterator().begin()->first));
					break;
				case 3:
					if (!selectedScene->sceneData.particleSystemRenderers.contains(selectedEntity->ID))
						selectedScene->registerComponent(selectedEntity->ID, ParticleSystemRenderer::ParticleSystemSize::Small);
					break;
				case 4:
					if (!selectedScene->sceneData.staticbodies.contains(selectedEntity->ID))
						selectedScene->registerComponent_Staticbody(selectedEntity->ID, Collider(vec2(1.0f)));
					break;
				case 5:
					if (!selectedScene->sceneData.staticbodies.contains(selectedEntity->ID))
						selectedScene->registerComponent_Staticbody(selectedEntity->ID, Collider(1.0f));
					break;
				case 6:
					if (!selectedScene->sceneData.rigidbodies.contains(selectedEntity->ID))
						selectedScene->registerComponent_Rigidbody(selectedEntity->ID, Collider(vec2(1.0f)));
					break;
				case 7:
					if (!selectedScene->sceneData.rigidbodies.contains(selectedEntity->ID))
						selectedScene->registerComponent_Rigidbody(selectedEntity->ID, Collider(1.0f));
					break;
				default:
					break;
				}
				comboSelected = -1;
			}

			InputString("Name", selectedEntity->name);
			Checkbox("Persistent", &selectedEntity->persistent);

			if (drawInspector(selectedEntity->transform)) {
				if (selectedScene->sceneData.rigidbodies.contains(selectedEntity->ID)) {
					auto body = &selectedScene->sceneData.rigidbodies.at(selectedEntity->ID);
					body->SetTransform(selectedEntity->transform.position, selectedEntity->transform.rotation);
					body->SetLinearVelocity(vec2(0.0f));
					body->SetAngularVelocity(0.0f);
				}
				if (selectedScene->sceneData.staticbodies.contains(selectedEntity->ID)) {
					selectedScene->sceneData.staticbodies.at(selectedEntity->ID).SetTransform(selectedEntity->transform.position, selectedEntity->transform.rotation);
				}
			}


			// color renderer inspector
			if (selectedScene->sceneData.colorRenderers.contains(selectedEntity->ID)) {
				SeparatorText("Color Renderer");
				if (Button("X##1"))
					selectedScene->sceneData.colorRenderers.erase(selectedEntity->ID);
				else
					drawInspector(selectedScene->sceneData.colorRenderers[selectedEntity->ID]);
			}

			// sprite renderer inspector
			if (selectedScene->sceneData.spriteRenderers.contains(selectedEntity->ID)) {
				SeparatorText("Sprite Renderer");
				if (Button("X##2")) {
					selectedScene->sceneData.spriteRenderers.erase(selectedEntity->ID);
				}
				else {
					rendererSelectedSprite = selectedScene->sceneData.spriteRenderers.at(selectedEntity->ID).sprite;
					drawInspector(selectedScene->sceneData.spriteRenderers.at(selectedEntity->ID));
				}
			}

			// text renderer inspector
			if (selectedScene->sceneData.textRenderers.contains(selectedEntity->ID)) {
				SeparatorText("Text Renderer");
				if (Button("X##3"))
					selectedScene->sceneData.textRenderers.erase(selectedEntity->ID);
				else
					drawInspector(selectedScene->sceneData.textRenderers[selectedEntity->ID]);
			}

			if (selectedScene->sceneData.particleSystemRenderers.contains(selectedEntity->ID)) {
				SeparatorText("Particle System");
				if (Button("X##4"))
					selectedScene->sceneData.particleSystemRenderers.erase(selectedEntity->ID);
				else
					drawInspector(selectedScene->sceneData.particleSystemRenderers.find(selectedEntity->ID)->second);
			}

			if (selectedScene->sceneData.staticbodies.contains(selectedEntity->ID)) {
				SeparatorText("Staticbody");
				if (Button("X##5")) {
				//	selectedScene->sceneData.staticbodies.at(selectedEntity->ID).Destroy();
					selectedScene->sceneData.staticbodies.erase(selectedEntity->ID);
				}
				else {
					drawInspector(selectedScene->sceneData.staticbodies.at(selectedEntity->ID));
				}
			}

			if (selectedScene->sceneData.rigidbodies.contains(selectedEntity->ID)) {
				SeparatorText("Rigidbody");
				if (Button("X##6")) {
					//selectedScene->sceneData.rigidbodies.at(selectedEntity->ID).Destroy();
					selectedScene->sceneData.rigidbodies.erase(selectedEntity->ID);
				}
				else {
					drawInspector(selectedScene->sceneData.rigidbodies.at(selectedEntity->ID));
				}
			}


			// behavior selector
			SeparatorText("Behavior");
			if (selectedScene->sceneData.behaviours.contains(selectedEntity->ID)) {
				if (Button("X##7")) {
					selectedScene->sceneData.behaviours.erase(selectedEntity->ID);
				}
				else {
					auto be = selectedScene->sceneData.behaviours.at(selectedEntity->ID).get();

					Text(BehaviorMap.at(be->Hash).first.c_str());

					auto props = be->getProperties();

					for (auto& prop : props)
					{
						switch (prop.type)
						{
						case SerializableProperty::Type::INT:
						{
							InputInt(prop.name.c_str(), reinterpret_cast<int*>(prop.value));
							break;
						}
						case SerializableProperty::Type::FLOAT:
						{
							InputFloat(prop.name.c_str(), reinterpret_cast<float*>(prop.value));
							break;
						}
						case SerializableProperty::Type::VEC2:
						{
							InputFloat2(prop.name.c_str(), reinterpret_cast<float*>(prop.value));
							break;
						}
						default:
							break;
						}
					}
				}
			}
			else {

				uint32_t selectedBehavior = 0;
				if (Button("Choose behavior")) {
					OpenPopup("Select Behavior");
					behaviorModel = true;
				}

				float modelWidth = 500;
				auto modelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
				if (behaviorModel) {
					SetNextWindowPos(vec2(glm::clamp(engine->winW / 2 - (int)modelWidth / 2, 0, engine->winW), 100));
					SetNextWindowSize(vec2(modelWidth, 600));
				}
				if (BeginPopupModal("Select Behavior", &behaviorModel, modelFlags)) {

					static char entSrchTxt[128];

					InputText("search behaviors", entSrchTxt, 128);


					string srchString(entSrchTxt);
					std::string lowerSearchQuery = toLower(srchString);

					int i = 0;
					for (auto& ent : BehaviorMap)
					{
						string behaviourName = ent.second.first;
						if (lowerSearchQuery.length() == 0 || (toLower(behaviourName).find(lowerSearchQuery) != std::string::npos)) {
							if (Selectable(behaviourName.c_str(), false)) {
								selectedBehavior = ent.first;
							}
						}
					}
					EndPopup();

					if (selectedBehavior != 0) {
						selectedScene->AddBehaviour(selectedEntity, selectedBehavior);
					}
				}
			}
		}
		else if (selectedSprite != nullptr) {

			vec2 avail = GetContentRegionAvail();
			avail.x -= 20; // room for scrollbar
			avail.y = 220;

			if (Combo("Filter mode", (int*)(&selectedSprite->filterMode), "Nearest\0Linear")) {
				engine->assetManager->UpdateSpritefilter(selectedSprite->ID);
			}

			if (selectedSprite->atlas.size() > 0) {
				SliderInt("Atlas index", &selectedSpriteAtlasIndex, 0, selectedSprite->atlas.size() - 1);
				DrawSpriteAtlas(selectedSprite->ID, avail, selectedSpriteAtlasIndex);
			}
			else {
				DrawSprite(selectedSprite->ID, avail);
			}
		}
		else if (sceneSelected) {
			InputString("Name", gameScene->name);
			vec4 col = engine->GetFramebufferClearColor(sceneFramebuffer);
			if (ColorPicker3("Background color", value_ptr(col))) {
				engine->SetFramebufferClearColor(sceneFramebuffer, col);
			}

			auto sceneNames = engine->assetManager->_getLoadedAndUnloadedSceneNames();
			bool sceneNameInUse = false;
			for (auto& s : sceneNames)
			{
				if (s == gameScene->name) {
					sceneNameInUse = true;
					break;
				}
			}

			bool save;
			if (sceneNameInUse)
				save = Button("Overwrite Save");
			else
				save = Button("Save");

			if (save) {

				std::filesystem::path dir(engine->assetManager->directories.sceneDir);
				std::filesystem::path fullPath = dir / std::filesystem::path(gameScene->name);
				engine->assetManager->ExportScene(gameScene, fullPath.string());
			}

		}
		End();
	}

	debugDataWindow();

	if (input->getMouseBtn(MouseBtn::Left) == false) {
		prefabDragInStarted = false;
	}

}



template<>
bool Editor::drawInspector<Transform>(Transform& t) {
	SeparatorText("Transform");
	bool change = false;
	change |= DragFloat2("position", value_ptr(t.position), 0.2f);
	change |= DragFloat2("scale", value_ptr(t.scale), 0.1f);
	change |= DragFloat("angle", &t.rotation, 0.01f);
	return change;
}

template<>
bool Editor::drawInspector<ColorRenderer>(ColorRenderer& t) {
	ColorPicker4("Color", value_ptr(t.color));
	{
		bool b = t.shape == ColorRenderer::Shape::Circle;
		if (Checkbox("Circle", &b)) {
			t.shape = b ? ColorRenderer::Shape::Circle : ColorRenderer::Shape::Rectangle;
		}

	}
	return false;
}

template<>
bool Editor::drawInspector<SpriteRenderer>(SpriteRenderer& r) {

	Checkbox("Use light map", &r.useLightMap);

	if (Button("choose asset")) {
		assetModel = true;
		OpenPopup("Available Assets");
	}

	auto modelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
	float modelWidth = 360;
	if (assetModel) {
		SetNextWindowPos(vec2(glm::clamp(engine->winW / 2 - (int)modelWidth / 2, 0, engine->winW), 50));
		SetNextWindowSize(vec2(modelWidth, glm::max(400, engine->winH - 100)));
	}
	if (BeginPopupModal("Available Assets", &assetModel, modelFlags)) {

		// make room for scrollbar
		const vec2 displaySize = vec2(modelWidth - 20, modelWidth);

		int i = 0;
		for (auto& sprite : engine->assetManager->_getSpriteIterator())
		{
			auto pos = GetCursorPos();
			vec2 dispSize = DrawSprite(sprite.first, displaySize);
			SetCursorPos(pos);
			if (InvisibleButton((string("invbtn") + to_string(i)).c_str(), displaySize)) {
				rendererSelectedSprite = sprite.first;
				selectedScene->sceneData.spriteRenderers.at(selectedEntity->ID)._spriteCache = engine->assetManager->GetSprite(rendererSelectedSprite);
				ImGui::CloseCurrentPopup();
			}
			i++;
		}
		EndPopup();
	}

	if (rendererSelectedSprite != r.sprite) {
		r.SetSprite(engine->assetManager->GetSprite(rendererSelectedSprite));
	}

	Text("Sprite ID: %d", r.sprite);

	const auto& sprite = engine->assetManager->GetSprite(r.sprite);

	Text("%d x %d", (int)sprite->resolution.x, (int)sprite->resolution.y);

	vec2 avail = GetContentRegionAvail();
	avail.x -= 20; // room for scrollbar
	avail.y = 220;

	if (sprite->atlas.size() > 0) {
		SliderInt("Atlas index", &r.atlasIndex, 0, sprite->atlas.size() - 1);
		DrawSpriteAtlas(sprite->ID, avail, r.atlasIndex);
	}

	DrawSprite(r.sprite, avail);
	return false;
}

template<>
bool Editor::drawInspector<TextRenderer>(TextRenderer& r) {

	vector<fontID> ids;
	vector<string> names;

	for (auto& [id, font] : engine->assetManager->_getFontIterator()) {
		ids.push_back(id);
		names.push_back(font.name);
	}

	int selected = indexOf(ids, r.font);
	if (Combo("Font", &selected, names)) {
		r.font = ids[selected];
		r.dirty = true;
	}

	r.dirty |= ImGui::InputString("Text", r.text);

	ColorPicker4("Text color", value_ptr(r.color));

	return false;
}


//InputFloatRange

bool Editor::drawInspector(ParticleSystemRenderer& r) {

	auto& ps = r.configuration;


	int tSize = static_cast<int>(r.size);
	if (Combo("Size", &tSize, "Small\0Large")) {
		assert(tSize == 0 || tSize == 1);
		r.SetSystemSize(static_cast<ParticleSystemRenderer::ParticleSystemSize>(tSize));
	}

	InputInt("Particle count", &ps.particleCount);
	if (r.size == ParticleSystemRenderer::ParticleSystemSize::Small) {
		ps.particleCount = glm::clamp(ps.particleCount, 0, MAX_PARTICLES_SMALL);
	}
	else if (r.size == ParticleSystemRenderer::ParticleSystemSize::Large) {
		ps.particleCount = glm::clamp(ps.particleCount, 0, MAX_PARTICLES_LARGE);
	}
	else {
		assert(false);
	}


	Checkbox("Burst mode", &ps.burstMode);
	Checkbox("Burst repeate", &ps.burstRepeat);
	InputFloatClamp("Spawn rate", &ps.spawnRate, 0.001);
	InputFloatClamp("Life span", &ps.particleLifeSpan, 0);
	InputFloat("Gravity acceleration", &ps.gravity);
	InputFloatClamp("Start size", &ps.startSize, 0);
	InputFloatClamp("End size", &ps.endSize, 0);
	ImGui::ColorEdit4("Start color", value_ptr(ps.startColor));
	ImGui::ColorEdit4("End color", value_ptr(ps.endColor));

	return false;
}

bool _drawInspector(Collider& collider) {

	if (collider.GetType() == Collider::Type::Box) {

		SeparatorText("Box Collider");
		vec2 nScale = collider.GetScale();
		bool change = ImGui::SliderFloat2("Scale", value_ptr(nScale), 0.0f, 100.0f);
		if (change) {
			nScale.x = glm::max(nScale.x, 0.01f);
			nScale.y = glm::max(nScale.y, 0.01f);
			collider.setScale(nScale);
		}
		return change;
	}
	else {
		SeparatorText("Circle Collider");
		float nrad = collider.GetRadius();
		bool change = ImGui::SliderFloat("Radius", &nrad, 0.0f, 100.0f);
		if (change) {
			nrad = glm::max(nrad, 0.01f);
			collider.setRadius(nrad);
		}
		return change;
	}
}

template<>
bool Editor::drawInspector<Rigidbody>(Rigidbody& r) {

	{
		{
			bool b = r.GetFixedRotation();
			if (Checkbox("fixed rotation", &b))
				r.SetFixedRotation(b);
		}
		SameLine();
		{
			bool b = r.GetBullet();
			if (Checkbox("bullet", &b))
				r.SetBullet(b);
		}

		InputFloatGetSetClamp("linear damping",
			[&]() { return r.GetLinearDamping(); }, [&](float f) { r.SetLinearDamping(f); }, 0.0f, 12.0f);

		InputFloatGetSetClamp("angular damping",
			[&]() { return r.GetAngularDamping(); }, [&](float f) { r.SetAngularDamping(f); }, 0.0f, 12.0f);

		InputFloatGetSetClamp("gravity scale",
			[&]() { return r.GetGravityScale(); }, [&](float f) { r.SetGravityScale(f); }, 0.0f, 10.0f);

		InputFloatGetSetClamp("friction",
			[&]() { return r.GetFriction(); }, [&](float f) { r.SetFriction(f); }, 0.0f, 10.0f);

		InputFloatGetSetClamp("density",
			[&]() { return r.GetDensity(); }, [&](float f) { r.SetDensity(f); }, 0.0f, 10.0f);

		InputFloatGetSetClamp("restitution",
			[&]() { return r.GetRestitution(); }, [&](float f) { r.SetRestitution(f); }, 0.0f, 1.0f);

	}

	if (_drawInspector(r.collider)) {
		r.updateCollider();
	}

	return false;
}

template<>
bool Editor::drawInspector<Staticbody>(Staticbody& r) {

	if (_drawInspector(r.collider)) {
		r.updateCollider();
	}

	return false;
}