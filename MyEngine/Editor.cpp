#define IM_VEC2_CLASS_EXTRA                                                 \
    ImVec2(const glm::vec2& f) { x = f.x; y = f.y; }                        \
    operator glm::vec2() const { return glm::vec2(x,y); }                   \
    ImVec2& operator+=(const glm::vec2& f) { x += f.x; y += f.y; return *this; } \
    ImVec2& operator-=(const glm::vec2& f) { x -= f.x; y -= f.y; return *this; }

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include"Engine.h"
#include "Physics.h"

#include "Editor.h"
#include "Utils.h"
#include "MyMath.h"
#include "BehaviorRegistry.h"
#include "FileDialog.h"

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


	float GetWorldGridSize(float zoomLevel) {
		if (zoomLevel < 0.02)
			return 10;
		if (zoomLevel < 0.1)
			return 1.0;
		if (zoomLevel < 0.5)
			return 1.0f / 2.0f;
		else
			return 1.0f / 10.0f;
	}

	float getWorldGridRounding(float value, float zoomLevel) {
		float gridSize = GetWorldGridSize(zoomLevel);
		return roundf(value / gridSize) * gridSize;
	}

	const float entityWindowWidth = 200;
	const float inspectorWindowWidth = 300;
}

vec2 Editor::DrawSprite(Engine& engine, spriteID id, glm::vec2 maxSize) {
	const auto& sprite = engine.assetManager->spriteAssets[id];
	const auto& texture = engine.assetManager->textureAssets[sprite->texture];

	if (texture.imTexture.has_value() == false) {
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

	Image((ImTextureID)texture.imTexture.value(), dispSize);
	return dispSize;
}
vec2 Editor::DrawSpriteAtlas(Engine& engine, spriteID id, glm::vec2 maxSize, int atlasIndex) {
	const auto& sprite = engine.assetManager->spriteAssets[id];
	const auto& texture = engine.assetManager->textureAssets[sprite->texture];

	if (texture.imTexture.has_value() == false) {
		Text("Could not display texture in Editor");
		return ImGui::GetItemRectSize();
	}

	if (sprite->Atlas.size() > 0) {
		auto atlasEntry = sprite->Atlas[atlasIndex];

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
		Image((ImTextureID)texture.imTexture.value(), dispSize, atlasEntry.uv_min, atlasEntry.uv_max);
		return dispSize;
	}
}



void Editor::DrawGrid(Engine& engine) {
	if (showGrid)
	{
		float gridSize = GetWorldGridSize(editorCamera.zoom);
		const int gridOpacity = (int)(0.2 * 255);

		{

			float x0 = engine.screenToWorldPos(vec2(0)).x;
			x0 = getWorldGridRounding(x0, editorCamera.zoom);
			x0 = engine.worldToScreenPos(vec2(x0, 0)).x;

			float inc = (gridSize * (screenSize.y / 2.0f)) * editorCamera.zoom;

			float xi = x0;
			while (xi <= screenSize.x) {
				drawlist->AddLine(vec2(xi, 0), vec2(xi, screenSize.y), IM_COL32(255, 255, 255, gridOpacity));
				xi += inc;
			}
		}
		{
			float y0 = engine.screenToWorldPos(vec2(0)).y;
			y0 = getWorldGridRounding(y0, editorCamera.zoom);
			y0 = engine.worldToScreenPos(vec2(0, y0)).y;

			float inc = (gridSize * (screenSize.y / 2.0f)) * editorCamera.zoom;

			float yi = y0;
			while (yi <= screenSize.x) {
				drawlist->AddLine(vec2(0, yi), vec2(screenSize.x, yi), IM_COL32(255, 255, 255, gridOpacity));
				yi += inc;
			}
		}
	}
}

void Editor::controlWindow(Engine& engine) {

	const float margin = 10.0f;
	auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
	SetNextWindowPos(vec2(entityWindowWidth + margin, margin));
	SetNextWindowSize(vec2(engine.getWindowSize().x - entityWindowWidth - inspectorWindowWidth - (margin * 2.0f), 40));
	Begin("control", nullptr, flags);

	if (engine.time - updateTimer > 0.2f) {
		frameRateStat = engine._getAverageFramerate();
		updateTimer = engine.time;
	}

	if (Button("Save Scene")) {
		engine.scene->SaveScene("gamescene");
	}
	SameLine();
	if (Button("Load Scene")) {
		engine.scene->LoadScene("gamescene", engine.bworld);
		selectedEntity = nullptr;
	}
	SameLine();
	if (Button("Show Stats")) {
		showingStats = true;
	}
	SameLine();
	Text("%.2f", frameRateStat);
	SameLine();
	if (engine.paused) {
		Text("paused");
	}
	else {
		Text("playing");
	}
	SameLine();
	Text("zoom: %.2f", editorCamera.zoom);

	End();
}

void Editor::entityWindow(Engine& engine) {

	auto flags = ImGuiWindowFlags_NoResize;
	SetNextWindowPos(vec2(0.0f, 0.0f));
	SetNextWindowSize(vec2(entityWindowWidth, engine.getWindowSize().y / 2));
	Begin("Entities", nullptr, flags);

	if (Button("new")) {
		engine.scene->RegisterEntity(make_shared<Entity>("", true));
	}

	int i = 0;
	for (auto& entity : engine.scene->sceneData.entities)
	{
		if (!entity.second->parent.has_value()) {

			if (Selectable(entity.second->name.c_str(), selectedEntityIndex == i)) {
				selectedEntityIndex = i;

				// already selected. deselect
				if (selectedEntity == entity.second) {
					selectedEntity = nullptr;
					selectedEntityIndex = -1;
				}
				// select new
				else {
					selectedEntity = entity.second;
				}
			}

			if (ImGui::BeginPopupContextItem()) {
				selectedEntityIndex = i;
				i++;
				if (ImGui::MenuItem("Delete")) {
					engine.scene->UnregisterEntity(entity.first);
					selectedEntity = nullptr;
					selectedEntityIndex = -1;
					ImGui::EndPopup();
					break;
				}
				if (ImGui::MenuItem("Duplicate")) {
					engine.scene->DuplicateEntity(entity.second);
					selectedEntity = nullptr;
					selectedEntityIndex = -1;
					ImGui::EndPopup();
					break;
				}
				if (ImGui::MenuItem("Save as prefab")) {
					Prefab p = engine.scene->CreatePrefab(entity.second);
					selectedEntity = nullptr;
					selectedEntityIndex = -1;
					ImGui::EndPopup();

					char filename[MAX_PATH];
					if (saveFileDialog(filename, MAX_PATH, "prefab", "Prefab File\0*.prefab\0")) {
						p.serializeJson(filename);
						engine.assetManager->loadPrefabs(engine.bworld);
					}

					break;
				}
				ImGui::EndPopup();
			}
			else {
				i++;
			}
			if (entity.second->children.size() > 0)
			{
				if (TreeNode("children")) {


					for (auto& child : entity.second->children)
					{
						if (Selectable(engine.scene->sceneData.entities[child]->name.c_str(), selectedEntityIndex == i)) {
							selectedEntityIndex = i;
							selectedEntity = engine.scene->sceneData.entities[child];
						}

						i++;
					}
					ImGui::TreePop();
				}
			}
		}
	}
	End();
}

void Editor::assetWindow(Engine& engine) {
	auto flags = ImGuiWindowFlags_NoResize;
	SetNextWindowPos(vec2(0.0f, engine.getWindowSize().y / 2.0f));
	SetNextWindowSize(vec2(entityWindowWidth, engine.getWindowSize().y / 2));
	Begin("Assets", nullptr, flags);

	if (BeginTabBar("assetCategories")) {

		if (BeginTabItem("Prefabs")) {
			int i = 0;
			for (auto& p : engine.assetManager->prefabs)
			{
				Selectable(p.first.c_str(), selectedPrefabIndex == i);

				if (ImGui::BeginPopupContextItem()) {
					selectedEntityIndex = i;
					i++;
					if (ImGui::MenuItem("Instantiate")) {
						engine.scene->Instantiate(p.second, p.first);
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
					auto sprite = engine.assetManager->GenerateSprite(string(filename));
					sprite->serializeJson(engine.assetManager->directories.assetDir + sprite->fileName + std::string(".sprite"));
				}
			}

			int i = 0;
			for (auto& p : engine.assetManager->spriteAssets) {

				if (p.first == engine.assetManager->defaultSprite) {
					if (Selectable("default sprite", selectedSpriteIndex == i)) {
						selectedSprite = p.second;
						selectedSpriteIndex = i;
						selectedSpriteAtlasIndex = 0;
						selectedEntity = nullptr;
					}
				}
				else {
					size_t lastindex = p.second->fileName.find_last_of(".");
					std::string rawname = p.second->fileName.substr(0, lastindex);
					if (Selectable(rawname.c_str(), selectedSpriteIndex == i)) {
						selectedSprite = p.second;
						selectedSpriteIndex = i;
						selectedSpriteAtlasIndex = 0;
						selectedEntity = nullptr;
					}
				}

				/*if (ImGui::BeginPopupContextItem()) {
					selectedEntityIndex = i;
					i++;
					if (ImGui::MenuItem("Instantiate")) {
						engine.scene->Instantiate(p.second, p.first);
						ImGui::EndPopup();
						break;
					}
					ImGui::EndPopup();
				}
				else {
					i++;
				}*/
				i++;
			}
			EndTabItem();
		}

		EndTabBar();
	}




	End();
}

void Editor::debugDataWindow(Engine& engine) {
	/*auto flags = ImGuiWindowFlags_NoResize;
	SetNextWindowPos(vec2(0.0f, engine.getWindowSize().y / 2.0f));
	SetNextWindowSize(vec2(entityWindowWidth, engine.getWindowSize().y / 2));*/
	//Begin("Stats", nullptr, flags);

	if (showingStats && Begin("Stats", &showingStats)) {
		auto& stats = engine._getDebugStats();

		Text("Entity count: %d", stats.entity_count);
		Text("Rendered sprited: %d", stats.sprite_render_count);

		End();
	}
}


void Editor::Run(Engine& engine) {

	allowZoomThisFrame = true;

	auto input = engine.GetInput();
	vec2 mpos = input->getMousePos();
	screenSize = engine.getWindowSize();

	drawlist = GetBackgroundDrawList();
	auto io = GetIO();

	if (io.WantTextInput == false) {

		if (input->getKeyDown('g'))
			showGrid = !showGrid;

		if (selectedEntity != nullptr && input->getKeyDown('f')) {
			camSlerpStart = editorCamera.position;
			camSlerpEnd = selectedEntity->transform.position;
			cameraEntityFocus = true;
			cameraSlepStartTime = engine.time;
		}
	}


	// panning
	{
		vec2 delta = mpos - lastMpos;
		lastMpos = mpos;

		if (input->getMouseBtn(MouseBtn::Right)) {
			editorCamera.position -= ((delta * vec2(2.0, -2.0)) / engine.getWindowSize().y) / editorCamera.zoom;
			cameraEntityFocus = false;
		}
	}

	if (cameraEntityFocus == true) {
		float fracComplete = (engine.time - cameraSlepStartTime) / 0.8f;
		editorCamera.position = smoothstep(camSlerpStart, camSlerpEnd, fracComplete);
		if (fracComplete >= 1.0f)
			cameraEntityFocus = false;
	}

	DrawGrid(engine);

	controlWindow(engine);

	entityWindow(engine);

	assetWindow(engine);


	{
		auto flags = ImGuiWindowFlags_NoResize;
		SetNextWindowPos(vec2(engine.getWindowSize().x - inspectorWindowWidth, 0.0f));
		SetNextWindowSize(vec2(300, engine.getWindowSize().y));
		Begin("Inspector", nullptr, flags);

		if (selectedEntity != nullptr) {


			// transform gizmo
			{
				float lineLen = 120; // translate gizmo arm length
				float wheelRad = 140; // rotate gizmo radius

				vec2 objScreenPos = engine.worldToScreenPos(selectedEntity->transform.position);
				vec2 yHandlePos = objScreenPos + vec2(0, -lineLen);
				vec2 xHandlePos = objScreenPos + vec2(lineLen, 0);

				if (input->getMouseBtnDown(MouseBtn::Left)) {
					draggingY = glm::distance(mpos, yHandlePos) < 16;
					draggingX = glm::distance(mpos, xHandlePos) < 16;

					// central handle
					if (glm::distance(mpos, objScreenPos) < 22) {
						draggingX = true;
						draggingY = true;
					}

					if ((draggingX || draggingY) == false) {
						float mDist = glm::distance(mpos, objScreenPos);
						if (mDist > wheelRad - 5 && mDist < wheelRad + 5) {
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
					float mouseWorldObjPos = engine.screenToWorldPos(mpos + (draggingX ? 0 : lineLen)).y;

					if (input->getKey(KeyCode::LeftControl))
						yHandlePos.y = roundf(mouseWorldObjPos * 100) / 100; // two decimal places
					else
						mouseWorldObjPos = getWorldGridRounding(mouseWorldObjPos, editorCamera.zoom);

					objScreenPos.y = engine.worldToScreenPos(vec2(0, mouseWorldObjPos)).y;
					yHandlePos.y = objScreenPos.y - lineLen;
				}

				if (draggingX) {

					float mouseWorldObjPos = engine.screenToWorldPos(mpos - (draggingY ? 0 : lineLen)).x;

					if (input->getKey(KeyCode::LeftControl))
						xHandlePos.x = roundf(mouseWorldObjPos * 100) / 100; // two decimal places
					else
						mouseWorldObjPos = getWorldGridRounding(mouseWorldObjPos, editorCamera.zoom);

					objScreenPos.x = engine.worldToScreenPos(vec2(mouseWorldObjPos, 0.0)).x;
					xHandlePos.x = objScreenPos.x + lineLen;

				}

				if (draggingX || draggingY) {
					vec2 objPos = engine.screenToWorldPos(objScreenPos);
					selectedEntity->transform.position = objPos;
					if (engine.scene->sceneData.rigidbodies.contains(selectedEntity->ID)) {
						engine.scene->sceneData.rigidbodies[selectedEntity->ID].SetPosition(objPos);
					}
					if (engine.scene->sceneData.staticbodies.contains(selectedEntity->ID)) {
						engine.scene->sceneData.staticbodies[selectedEntity->ID].SetPosition(objPos);
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
					if (engine.scene->sceneData.rigidbodies.contains(selectedEntity->ID)) {
						engine.scene->sceneData.rigidbodies[selectedEntity->ID].SetRotation(selectedEntity->transform.rotation);
					}
					if (engine.scene->sceneData.staticbodies.contains(selectedEntity->ID)) {
						engine.scene->sceneData.staticbodies[selectedEntity->ID].SetRotation(selectedEntity->transform.rotation);
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



			if (Combo("Create", &comboSelected, "Sprite Renderer\0Color Renderer\0Box Staticbody\0Circle Staticbody\0Box Rigidbody\0Circle Rigidbody")) {
				switch (comboSelected)
				{
				case 0:
					if (!engine.scene->sceneData.spriteRenderers.contains(selectedEntity->ID)) {
						engine.scene->registerComponent(selectedEntity->ID, SpriteRenderer(engine.assetManager->defaultSprite));
					}
					break;
				case 1:
					if (!engine.scene->sceneData.colorRenderers.contains(selectedEntity->ID)) {
						engine.scene->registerComponent(selectedEntity->ID, ColorRenderer(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
					}
					break;
				case 2:
					if (!engine.scene->sceneData.staticbodies.contains(selectedEntity->ID)) {
						engine.scene->registerComponent(selectedEntity->ID, Staticbody(engine.bworld, make_shared<BoxCollider>(vec2(1.0f))));
					}
					break;
				case 3:
					if (!engine.scene->sceneData.staticbodies.contains(selectedEntity->ID)) {
						engine.scene->registerComponent(selectedEntity->ID, Staticbody(engine.bworld, make_shared<CircleCollider>(1.0f)));
					}
					break;
				case 4:
					if (!engine.scene->sceneData.rigidbodies.contains(selectedEntity->ID)) {
						engine.scene->registerComponent(selectedEntity->ID, Rigidbody(make_shared<BoxCollider>(vec2(1.0f))));
					}
					break;
				case 5:
					if (!engine.scene->sceneData.rigidbodies.contains(selectedEntity->ID)) {
						engine.scene->registerComponent(selectedEntity->ID, Rigidbody(make_shared<CircleCollider>(1.0f)));
					}
					break;
				default:
					break;
				}
			}

			InputString("Name", selectedEntity->name);
			Checkbox("Persistent", &selectedEntity->persistent);

			if (drawInspector(selectedEntity->transform, engine)) {
				if (engine.scene->sceneData.rigidbodies.contains(selectedEntity->ID)) {
					engine.scene->sceneData.rigidbodies[selectedEntity->ID].SetTransform(selectedEntity->transform.position, selectedEntity->transform.rotation);
				}
			}


			if (engine.scene->sceneData.colorRenderers.contains(selectedEntity->ID)) {
				drawInspector(engine.scene->sceneData.colorRenderers[selectedEntity->ID], engine);
			}

			if (engine.scene->sceneData.spriteRenderers.contains(selectedEntity->ID)) {
				rendererSelectedSprite = engine.scene->sceneData.spriteRenderers[selectedEntity->ID].sprite;
				drawInspector(engine.scene->sceneData.spriteRenderers[selectedEntity->ID], engine);
			}

			if (engine.scene->sceneData.staticbodies.contains(selectedEntity->ID)) {
				drawInspector(engine.scene->sceneData.staticbodies[selectedEntity->ID], engine);
			}

			if (engine.scene->sceneData.rigidbodies.contains(selectedEntity->ID)) {
				drawInspector(engine.scene->sceneData.rigidbodies[selectedEntity->ID], engine);
			}




			// behavior selector
			SeparatorText("Behavior");
			if (selectedEntity->GetEditorName() != "") {
				Text(selectedEntity->GetEditorName().c_str());
			}
			else {

				uint32_t selectedBehavior = 0;
				if (Button("Choose behavior")) {
					OpenPopup("Available Behaviors");
					behaviorModel = true;
				}

				if (BeginPopupModal("Available Behaviors", &behaviorModel)) {

					static char entSrchTxt[128];

					InputText("search behaviors", entSrchTxt, 128);


					string srchString(entSrchTxt);

					int i = 0;
					for (auto& ent : BehaviorMap)
					{
						if (strlen(entSrchTxt) == 0 || (srchString.find(ent.second.first) != std::string::npos)) {
							if (Selectable(ent.second.first.c_str(), false)) {
								selectedBehavior = ent.first;
							}
						}
					}
					EndPopup();

					if (selectedBehavior != 0) {
						//engine.scene->OverwriteEntity(BehaviorMap[selectedBehavior].second(), selectedEntity->ID);
						auto behaviorEntity = BehaviorMap[selectedBehavior].second();
						behaviorEntity->transform = selectedEntity->transform;
						engine.scene->OverwriteEntity(behaviorEntity, selectedEntity->ID);
						selectedEntity = behaviorEntity;
					}
				}
			}
		}
		else if (selectedSprite != nullptr) {



			vec2 avail = GetContentRegionAvail();
			avail.x -= 20; // room for scrollbar
			avail.y = 220;

			auto& texAsset = engine.assetManager->textureAssets[selectedSprite->texture];

			if (Combo("Filter mode", &selectedSpriteFilterCombo, "Nearest\0Linear")) {
				selectedSprite->filterMode = (FilterMode)selectedSpriteFilterCombo;
				engine.assetManager->updateTexture(selectedSprite->texture, selectedSprite->filterMode);
			}

			if (selectedSprite->Atlas.size() > 0) {
				SliderInt("Atlas index", &selectedSpriteAtlasIndex, 0, selectedSprite->Atlas.size() - 1);
				DrawSpriteAtlas(engine, selectedSprite->ID, avail, selectedSpriteAtlasIndex);
			}
			else {
				DrawSprite(engine, selectedSprite->ID, avail);
			}
		}
		End();
	}

	debugDataWindow(engine);

	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
		allowZoomThisFrame = false;
	}

	if (allowZoomThisFrame) {
		float off = input->GetScrollDelta();
		zoomP += off * 0.02f;
		zoomP = glm::clamp(zoomP, 0.0f, 1.0f);
		editorCamera.zoom = exponentialScale(zoomP, 0.01, 15.1);
	}
}



template<>
bool Editor::drawInspector<Transform>(Transform& t, Engine& engine) {
	SeparatorText("Transform");
	bool change = false;
	change |= DragFloat2("position", value_ptr(t.position), 0.2f);
	change |= DragFloat2("scale", value_ptr(t.scale), 0.1f);
	change |= DragFloat("angle", &t.rotation, 0.01f);
	return change;
}

template<>
bool Editor::drawInspector<ColorRenderer>(ColorRenderer& t, Engine& engine) {
	SeparatorText("Color Renderer");
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
bool Editor::drawInspector<SpriteRenderer>(SpriteRenderer& r, Engine& engine) {
	SeparatorText("Sprite Renderer");

	if (Button("choose asset")) {
		assetModel = true;
		OpenPopup("Available Assets");
	}

	auto modelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	float modelWidth = 360;
	if (assetModel) {
		SetNextWindowPos(vec2(glm::clamp(engine.winW / 2 - (int)modelWidth / 2, 0, engine.winW), 50));
		SetNextWindowSize(vec2(modelWidth, glm::max(400, engine.winH -100)));
	}
	if (BeginPopupModal("Available Assets", &assetModel, modelFlags)) {

		// make room for scrollbar
		const vec2 displaySize = vec2(modelWidth - 20, modelWidth);

		int i = 0;
		for (auto& sprite : engine.assetManager->spriteAssets)
		{
			auto pos = GetCursorPos();
			vec2 dispSize = DrawSprite(engine, sprite.first, displaySize);
			SetCursorPos(pos);
			if (InvisibleButton((string("invbtn") + to_string(i)).c_str(), displaySize)) {
				rendererSelectedSprite = sprite.first;
				ImGui::CloseCurrentPopup();
			}
			i++;
		}
		EndPopup();
	}

	if (rendererSelectedSprite != r.sprite) {
		r.sprite = rendererSelectedSprite;
		engine.assetManager->spritesAdded = true;
	}

	Text("Sprite ID: %d", r.sprite);

	const auto& sprite = engine.assetManager->spriteAssets[r.sprite];
	const auto& texture = engine.assetManager->textureAssets[sprite->texture];

	Text("%d x %d", (int)sprite->resolution.x, (int)sprite->resolution.y);

	vec2 avail = GetContentRegionAvail();
	avail.x -= 20; // room for scrollbar
	avail.y = 220;

	if (sprite->Atlas.size() > 0) {
		SliderInt("Atlas index", &r.atlasIndex, 0, sprite->Atlas.size() - 1);
		DrawSpriteAtlas(engine, sprite->ID, avail, r.atlasIndex);
	}

	DrawSprite(engine, r.sprite, avail);
	return false;
}


bool CircleCollider::drawInspector() {
	SeparatorText("Circle Collider");
	bool change = ImGui::SliderFloat("Radius", &radius, 0.0f, 100.0f);
	if (change) {
		radius = glm::max(radius, 0.01f);
	}
	return change;
}
bool BoxCollider::drawInspector() {
	SeparatorText("Box Collider");
	vec2 nScale = scale; //?
	bool change = ImGui::SliderFloat2("Scale", value_ptr(nScale), 0.0f, 100.0f);
	scale = nScale;
	if (change) {
		scale.x = glm::max(scale.x, 0.01f);
		scale.y = glm::max(scale.y, 0.01f);
	}
	return change;
}

template<>
bool Editor::drawInspector<Rigidbody>(Rigidbody& r, Engine& engine) {
	SeparatorText("Rigidbody");

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

	if (r.collider->drawInspector()) {
		r.updateCollider();
	}

	return false;
}

template<>
bool Editor::drawInspector<Staticbody>(Staticbody& r, Engine& engine) {
	SeparatorText("Staticbody");


	if (r.collider->drawInspector()) {
		r.updateCollider();
	}

	return false;
}