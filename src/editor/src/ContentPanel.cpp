#include "ContentPanel.hpp"
#include "Components/MeshReference.hpp"
#include "Components/WorldTransform.hpp"
#include "FileMetadata.hpp"
#include "IFile.hpp"
#include "Logger.hpp"
#include "Query.hpp"
#include "Ref.hpp"
#include "ResourceManager.hpp"
#include "Result.hpp"
#include "Loaders/GltfLoadFunctions.hpp"
#include "UI.hpp"
#include "VirtualFilesystem.hpp"
#include "WindowManager.hpp"
#include "crypto/Hashing.hpp"
#include "Assertions.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include "serialization/Serialization.hpp"
#include <cstddef>
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <filesystem>
#include <imgui/imgui.h>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <span>
#include <string>

constexpr ImGuiWindowFlags CONTENT_PANEL_FLAGS = ImGuiViewportFlags_NoFocusOnAppearing;

void Hush::ContentPanel::Init(Scene *activeScene) noexcept
{
	activeScene->CreateQuery<ResourceManager, VirtualFilesystem>().Each([this](Entity& entt, ResourceManager& resourceManager, VirtualFilesystem& vfs) {
		resourceManager.Init(&vfs);
		this->m_resourceManager = &resourceManager;
		this->m_filesystem = &vfs;
	});
	this->m_scene = activeScene;
	// this->m_folderImage = this->m_resourceManager->LoadTexture("engine_res://folder.png");
	// this->m_fileImage = this->m_resourceManager->LoadTexture("engine_res://file.png");
	this->m_modelLoader.SetResourceManager(this->m_resourceManager, this->m_filesystem);
}

void Hush::ContentPanel::OnRender() {
	if (ImGui::Begin("Project", nullptr, CONTENT_PANEL_FLAGS))
	{        
		if (this->m_dirty) {
			this->RefreshDirectory();
			// this->GenerateMetaFiles();
			UI::S_INITIALIZED = true;
		}
		ImGui::Text("Current Working Directory: %s", this->m_currentWorkingDirectory.c_str());
		bool isMouseInScene = !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		this->DrawFiles(isMouseInScene);
		const ImGuiPayload *payload = ImGui::GetDragDropPayload();
		if (isMouseInScene && payload != nullptr && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			const auto* data = reinterpret_cast<const FileInfo*>(payload->Data);
			if (CanBeDroppedToScene(*data)) {
				IRenderer* renderer = WindowManager::GetMainWindow()->GetInternalRenderer();
				auto result = this->m_modelLoader.LoadMeshes(renderer, data->path, this->m_scene);
				HUSH_RESULT_ASSERT(result, "Failed to load meshes!");
				// Use the Model Loader interface to get entities and then forward that to the renderer
				LogFormat(ELogLevel::Info, "Dropped payload {}!", data->path.filename().string());
				// Very very bad code, we should change it before a PR
				for (Entity& entt : result.value()) {
					renderer->PushMesh(entt.GetComponent<WorldTransform>(), entt.GetComponent<MeshReference>()->GetMesh().Get());
				}
			}
		}
	}
	ImGui::End();
}


void Hush::ContentPanel::DrawFiles(bool isMouseInScene) {
	ImVec2 regionDimensions = ImGui::GetContentRegionAvail();
	
    float regionWidth = regionDimensions.x;

    ImGuiStyle& style = ImGui::GetStyle();
    float spacing = style.ItemSpacing.x;
    float cursorX = 0.0F;
	for (const FileInfo & item : this->m_currentItems) {
		// TODO: Fix all the copies that this makes
		const std::string& fileName = item.path.filename().string();
		ImVec2 textSize = ImGui::CalcTextSize(fileName.c_str());
        float buttonWidth = textSize.x + style.FramePadding.x * 2.0F;

        // If this button would exceed the region width, wrap to next line
        if (cursorX + buttonWidth > regionWidth) {
            ImGui::NewLine();
            cursorX = 0.0F;
        }

        // Draw the button as a draggable source
        ImGui::PushID(fileName.c_str());
        if (ImGui::Button(fileName.c_str(), ImVec2(buttonWidth, 50.0F))) {
            // TODO: Handle the click stuff
        }
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", &item, sizeof(FileInfo));
            if (isMouseInScene && CanBeDroppedToScene(item)) {
            	ImGui::Text("Import to scene...");
            }
            else {
                ImGui::Text("Dragging \"%s\"", fileName.c_str());
            }
            ImGui::EndDragDropSource();
        }
        ImGui::PopID();

        // Advance cursor and prepare for next same-line
        cursorX += buttonWidth + spacing;
        ImGui::SameLine(0.0F, spacing);
	}
}

bool Hush::ContentPanel::CanBeDroppedToScene(const FileInfo& fileData) const {
	return fileData.flags == EFileFlags::File && fileData.IsModelFile();
}

void Hush::ContentPanel::RefreshDirectory() {
	this->m_currentItems.clear();
	this->m_currentItems = this->m_filesystem->ListPath(this->m_currentWorkingDirectory);
	this->m_dirty = false;
}


void Hush::ContentPanel::GenerateMetaFiles() {
	// Scan directories recursively and query the database indexing (NYI)
	for(const FileInfo& data : this->m_currentItems) {
		if (!data.ShouldGenerateMetaFile() || data.extension == EFileExtension::META) {
			continue;
		}

		FileMetadata metadata = {
			.metadataVersion = FileMetadata::VERSION,
			.id = Hashing::Fnv1a(data.path.string())
		};

		// Create inner resource files
		this->CreateInnerResources(data, metadata);
		

		// this->MakeMetaFile(data, metadata);
	}
}

void Hush::ContentPanel::MakeMetaFile(const FileInfo& fileData, const FileMetadata& metadata) {
	std::filesystem::path metaPath = fileData.path.parent_path() / fileData.path.stem().string().append(".meta");
	if (std::filesystem::exists(metaPath)) {
		return;
	}
	Result<std::unique_ptr<IFile>, IFile::EError> openRes = this->m_filesystem->OpenFile(metaPath.string(), EFileOpenMode::Write);
	HUSH_RESULT_ASSERT(openRes, "Failed to create metadata file!");

	std::unique_ptr<IFile>& file = openRes.value();
	Result<std::string, Serialization::ESerializationError> serializationResult = Serialization::SerializeJson(metadata);
	HUSH_RESULT_ASSERT(serializationResult, "Could not serialize metadata for file {}", fileData.path.string());

	std::string& json = serializationResult.value();
	std::span<const std::byte> writeBuffer(reinterpret_cast<const std::byte*>(json.data()), json.size());
	Result<void, IFile::EError> writeRes = file->Write(writeBuffer);
	HUSH_RESULT_ASSERT(writeRes, "Failed to write metadata file");

	file->Close();
}

void Hush::ContentPanel::CreateInnerResources(const FileInfo& fileData, const FileMetadata& metadata) {
	switch (fileData.extension) {

	case EFileExtension::UNKWOWN:
	case EFileExtension::PNG:
	case EFileExtension::JPEG:
	case EFileExtension::TXT:
	case EFileExtension::PDF:
	case EFileExtension::CSHARP:
	case EFileExtension::CPP:
	case EFileExtension::FBX:
	case EFileExtension::GLTF:
		break;
	case EFileExtension::GLB:
		fastgltf::Expected<fastgltf::Asset> asset = GltfLoadFunctions::GetAssetFromFile(fileData.path);
		HUSH_ASSERT(asset, "GLTF asset at {} not properly loaded, error: {}!", fileData.path.string(), fastgltf::getErrorMessage(asset.error()));
		size_t cntr = 0;
		// TODO: Swap for regular for loop
		for (const fastgltf::Image& image : asset->images) {
			// Write the binary data to the png
			
			fastgltf::MimeType mimeType = fastgltf::MimeType::None;
			const std::span<const std::byte> imageBuffer = GltfLoadFunctions::ExtractImageBuffer(image, asset.get(), &mimeType);
			if (imageBuffer.empty()) {
				continue;
			}
			std::filesystem::path parentDir = fileData.path.parent_path();
			std::string textName;
			if (image.name.empty()) {
				// I know, I know
				textName = fileData.path.stem().string()
					.append("_")
					.append(std::to_string(cntr))
					.append(".")
					.append(magic_enum::enum_name(mimeType));
			}
			else {
				textName = image.name;
			}
			std::filesystem::path generatedFileName = parentDir / textName;
			Result<std::unique_ptr<IFile>, IFile::EError> createFileRes = this->m_filesystem->OpenFile(generatedFileName.string(), EFileOpenMode::Write);
			HUSH_RESULT_ASSERT(createFileRes, "Failed to create inner resource for asset {}, on resource {}", fileData.path, image.name);
			std::unique_ptr<IFile>& createdFile = createFileRes.value();
			Result<void, IFile::EError> writeResult = createdFile->Write(imageBuffer);
			HUSH_RESULT_ASSERT(writeResult, "Failed to write image buffer");
			createdFile->Close();
			
			FileMetadata metadata = {
				.metadataVersion = FileMetadata::VERSION,
				.id = Hashing::Fnv1a(createdFile->GetFileInfo().path.string())
			};
			// this->MakeMetaFile(createdFile->GetFileInfo(), metadata);
			// this->m_currentItems.emplace_back(createdFile->GetFileInfo());
			cntr++;
		}
		break;
	}
}

