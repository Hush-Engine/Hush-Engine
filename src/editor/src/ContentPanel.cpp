#include "ContentPanel.hpp"
#include "Components/GlobalKeys.hpp"
#include "Entity.hpp"
#include "FileMetadata.hpp"
#include "IFile.hpp"
#include "Query.hpp"
#include "Result.hpp"
#include "UI.hpp"
#include "VirtualFilesystem.hpp"
#include "components/EditorInfo.hpp"
#include "Components/GpuUploadComponent.hpp"
#include "Components/Material3D.hpp"
#include "Entity.hpp"
#include "IFile.hpp"
#include "Query.hpp"
#include "RHI/IGraphicsDevice.hpp"
#include "RHI/IGraphicsTexture.hpp"
#include "Result.hpp"
#include "Scene.hpp"
#include "UI.hpp"
#include "VirtualFilesystem.hpp"
#include "WindowRenderer.hpp"
#include "components/EditorInfo.hpp"
#include "Loaders/GltfLoader.hpp"
#include "crypto/Hashing.hpp"
#include "Assertions.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include "serialization/Serialization.hpp"
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <imgui/imgui.h>
#include <string>
#include <vector>
#include "HushEngine.hpp"
#include "systems/RenderingSystem.hpp"

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
		if (isMouseInScene && payload != nullptr && payload->DataSize == sizeof(DroppableFile) &&
			ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			const auto *data = reinterpret_cast<const DroppableFile *>(payload->Data);
			if (CanBeDroppedToScene(*data))
			{
				Entity renderingSystemEnt = this->m_scene->CreateEntityWithKey("RenderingSystem");
				auto *systemRef = *renderingSystemEnt.GetComponent<RenderingSystem *>();

				HushEngine *engine = this->m_scene->GetEngine();
				Graphics::IGraphicsDevice *device = engine->GetWindowRenderer()->GetGraphicsDevice();
				RenderingContext ctx = {.materialDescriptor = &systemRef->GetPBRDescriptor(),
										.activeScene = this->m_scene,
										.resourceManager = this->m_resourceManager,
										.virtualFilesystem = this->m_filesystem,
										.device = device};

				Entity rootEntity = GLTFLoader::GenerateMeshEntities(ctx, data->virtualPath);
				HUSH_ASSERT(rootEntity.IsValid(), "Failed to create meshes from the GLTF file!");
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
		// The holder is tracked and destroyed once the upload completes.
		Entity holder = this->m_scene->CreateEntity();
		holder.EmplaceComponent<Ref<TextureComponent>>(texture);
		this->m_thumbnailHolders.emplace(vpath, std::move(holder));

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

	// Upload completed: the holder entity has served its purpose. Wait until the
	// ResourceUploadSystem has also removed GpuUploadComponent (OnPostRender) so we
	// never destroy an entity the upload system still references this frame.
	auto holderIt = this->m_thumbnailHolders.find(vpath);
	if (holderIt != this->m_thumbnailHolders.end() && !holderIt->second.HasComponent<Renderer::GpuUploadComponent>())
	{
		this->m_scene->DestroyEntity(holderIt->second);
		this->m_thumbnailHolders.erase(holderIt);
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

			this->m_currentDroppable = {.virtualPath = this->m_currentWorkingDirectory + fileName, .fileInfo = &item};

			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", &this->m_currentDroppable, sizeof(DroppableFile));
			if (isMouseInScene && CanBeDroppedToScene(this->m_currentDroppable))
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

bool Hush::ContentPanel::CanBeDroppedToScene(const DroppableFile &fileData) const
{
	return fileData.fileInfo->flags == EFileFlags::File && fileData.fileInfo->IsModelFile();
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
