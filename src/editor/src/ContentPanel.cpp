#include "ContentPanel.hpp"
#include "Components/GlobalKeys.hpp"
#include "Entity.hpp"
#include "IFile.hpp"
#include "Query.hpp"
#include "RHI/IGraphicsTexture.hpp"
#include "Scene.hpp"
#include "UI.hpp"
#include "VirtualFilesystem.hpp"
#include "components/EditorInfo.hpp"
#include <cctype>
#include <filesystem>
#include <imgui/imgui.h>
#include <string>
#include <vector>
#include "HushEngine.hpp"

constexpr ImGuiWindowFlags CONTENT_PANEL_FLAGS = ImGuiWindowFlags_NoFocusOnAppearing;

void Hush::ContentPanel::Init(Scene *activeScene) noexcept
{
	this->m_resourceManager = activeScene->GetEngine()->GetResourceManager();
	this->m_filesystem = activeScene->GetEngine()->GetVirtualFilesystem();
	this->m_scene = activeScene;
	Entity editorInfoEntity = activeScene->CreateEntityWithKey(ENGINE_MANAGER);
	this->m_editorInfoRef = editorInfoEntity.CreateComponentReference<EditorInfo>();
	// this->m_folderImage = this->m_resourceManager->LoadTexture("engine_res://folder.png");
	// this->m_fileImage = this->m_resourceManager->LoadTexture("engine_res://file.png");
	// this->m_modelLoader.SetResourceManager(this->m_resourceManager);
}

void Hush::ContentPanel::OnRender([[maybe_unused]] float deltaTime)
{
	if (ImGui::Begin("Project", nullptr, CONTENT_PANEL_FLAGS))
	{
		this->MarkDirtyIfContentChanged();
		if (this->m_dirty)
		{
			this->RefreshDirectory();
			UI::S_INITIALIZED = true;
		}
		ImGui::Text("Current Working Directory: %s", this->m_currentWorkingDirectory.c_str());
		bool isMouseInScene = this->m_editorInfoRef.GetData<EditorInfo>()->isMouseOnScene;
		this->DrawFiles(isMouseInScene);
		const ImGuiPayload *payload = ImGui::GetDragDropPayload();
		if (isMouseInScene && payload != nullptr && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			const auto *data = reinterpret_cast<const FileInfo *>(payload->Data);
			if (CanBeDroppedToScene(*data))
			{
				// GLTFLoader::GenerateMeshEntities(this->m_scene, this->m_resourceManager, data->path);
				// Create the mesh resources (?
				// A mesh is just data, we can represent that on disk (except GPUMeshBuffers)
				// GLBs and other model files have hierarchy data attached to them, we need a way to handle that

				// auto result = this->m_modelLoader.LoadMeshes(renderer, data->path, this->m_scene);
				// HUSH_RESULT_ASSERT(result, "Failed to load meshes!");
				// // Use the Model Loader interface to get entities and then forward that to the renderer
				// LogFormat(ELogLevel::Info, "Dropped payload {}!", data->path.filename().string());
				// // Very very bad code, we should change it before a PR
				// for (Entity &entt : result.value())
				// {
				// 	renderer->PushMesh(entt.GetComponent<WorldTransform>(),
				// 					   entt.GetComponent<MeshReference>()->GetMesh().Get());
				// }
			}
		}
	}
	ImGui::End();
}

bool Hush::ContentPanel::IsImageExtension(EFileExtension ext)
{
	return ext == EFileExtension::PNG || ext == EFileExtension::JPEG;
}

Hush::TextureComponent *Hush::ContentPanel::ResolveThumbnail(const std::string &vpath)
{
	auto it = this->m_thumbnailCache.find(vpath);
	if (it == this->m_thumbnailCache.end())
	{
		if (this->m_resourceManager == nullptr || this->m_scene == nullptr)
		{
			return nullptr;
		}

		// Downscale to a small thumbnail so we don't upload full-resolution textures.
		constexpr uint32_t THUMBNAIL_MAX_PIXELS = 128;
		auto result = this->m_resourceManager->LoadTexture(
			vpath, TextureComponent::ECpuUnloadStrategy::UnloadAfterUpload, THUMBNAIL_MAX_PIXELS);
		if (result.has_error())
		{
			// Cache a null Ref so a failed/missing image isn't retried every frame.
			this->m_thumbnailCache.emplace(vpath, Ref<TextureComponent>{});
			return nullptr;
		}

		Ref<TextureComponent> texture = result.value();

		// Attach the Ref to a lightweight, unnamed entity (no transform/name, so it stays
		// out of the hierarchy and is never rendered). Adding a Ref<TextureComponent> is
		// what triggers the ResourceUploadSystem's observer to schedule the GPU upload.
		Entity holder = this->m_scene->CreateEntity();
		holder.EmplaceComponent<Ref<TextureComponent>>(texture);

		it = this->m_thumbnailCache.emplace(vpath, std::move(texture)).first;
	}

	if (it->second.IsNull())
	{
		return nullptr;
	}

	TextureComponent *component = it->second.Get();
	// GetGpuTexture() is null until the async upload completes; show a placeholder until then.
	if (component == nullptr || component->GetGpuTexture() == nullptr)
	{
		return nullptr;
	}
	return component;
}

void Hush::ContentPanel::DrawFiles(bool isMouseInScene)
{
	ImGuiStyle &style = ImGui::GetStyle();

	// Thumbnail cell metrics.
	constexpr float THUMBNAIL_SIZE = 72.0F;
	const float cellStride = THUMBNAIL_SIZE + style.ItemSpacing.x;

	const float regionWidth = ImGui::GetContentRegionAvail().x;
	int columnCount = static_cast<int>(regionWidth / cellStride);
	if (columnCount < 1)
	{
		columnCount = 1;
	}

	int columnIndex = 0;
	for (const FileInfo &item : this->m_currentItems)
	{
		const std::string fileName = item.path.filename().string();

		ImGui::PushID(fileName.c_str());
		ImGui::BeginGroup();

		bool thumbnailDrawn = false;
		if (IsImageExtension(item.extension))
		{
			const std::string vpath = this->m_currentWorkingDirectory + fileName;
			if (TextureComponent *tex = this->ResolveThumbnail(vpath))
			{
				// The ImGui WebGPU backend takes a WGPUTextureView cast to ImTextureID.
				auto texId = reinterpret_cast<ImTextureID>(tex->GetGpuTexture()->GetNativeView());
				ImGui::ImageButton("##thumb", texId, ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
				thumbnailDrawn = true;
			}
		}

		if (!thumbnailDrawn)
		{
			// Placeholder tile for non-image assets, or while an image is still uploading.
			// Label it with the (dot-less, upper-cased) extension as a lightweight file icon.
			std::string ext = item.path.extension().string();
			if (!ext.empty() && ext.front() == '.')
			{
				ext.erase(ext.begin());
			}
			for (char &ch : ext)
			{
				ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
			}
			ImGui::Button(ext.empty() ? "?" : ext.c_str(), ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
		}

		// Drag-and-drop source (e.g. drag a model onto the scene).
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", &item, sizeof(FileInfo));
			if (isMouseInScene && CanBeDroppedToScene(item))
			{
				ImGui::Text("Import to scene...");
			}
			else
			{
				ImGui::Text("Dragging \"%s\"", fileName.c_str());
			}
			ImGui::EndDragDropSource();
		}

		// File name under the tile, wrapped to the cell width.
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + THUMBNAIL_SIZE);
		ImGui::TextWrapped("%s", fileName.c_str());
		ImGui::PopTextWrapPos();

		ImGui::EndGroup();
		ImGui::PopID();

		if (++columnIndex < columnCount)
		{
			ImGui::SameLine();
		}
		else
		{
			columnIndex = 0;
		}
	}
}

bool Hush::ContentPanel::CanBeDroppedToScene(const FileInfo &fileData) const
{
	return fileData.flags == EFileFlags::File && fileData.IsModelFile();
}

void Hush::ContentPanel::RefreshDirectory()
{
	this->m_currentItems.clear();
	std::vector<FileInfo> items = this->m_filesystem->ListPath(this->m_currentWorkingDirectory);

	// Hide cooker bookkeeping from the Project view: the .hmeta sidecars and the
	// .hcooked cache directory.
	for (FileInfo &item : items)
	{
		if (item.extension == EFileExtension::HMETA || item.path.filename() == ".hcooked")
		{
			continue;
		}
		this->m_currentItems.push_back(std::move(item));
	}

	this->m_dirty = false;
}

void Hush::ContentPanel::MarkDirtyIfContentChanged()
{
	if (this->m_filesystem == nullptr)
	{
		return;
	}

	// Map the current virtual directory to its backing OS path. Adding/removing/renaming
	// a file updates the directory's modification time, so we re-list only when it changes.
	auto hostDir = this->m_filesystem->ResolveHostPath(this->m_currentWorkingDirectory);
	if (hostDir.has_error())
	{
		return;
	}

	std::error_code ec;
	const auto writeTime = std::filesystem::last_write_time(hostDir.value(), ec);
	if (ec)
	{
		return;
	}

	if (writeTime != this->m_lastContentWriteTime)
	{
		this->m_lastContentWriteTime = writeTime;
		this->m_dirty = true;
	}
}
