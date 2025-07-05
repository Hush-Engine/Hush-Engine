#include "ContentPanel.hpp"
#include "FileMetadata.hpp"
#include "IFile.hpp"
#include "Logger.hpp"
#include "Query.hpp"
#include "ResourceManager.hpp"
#include "Result.hpp"
#include "VirtualFilesystem.hpp"
#include "WindowManager.hpp"
#include "crypto/Hashing.hpp"
#include "imgui/imgui_internal.h"
#include "Assertions.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include "serialization/Serialization.hpp"
#include <cstddef>
#include <filesystem>
#include <imgui/imgui.h>
#include <memory>
#include <span>
#include <string_view>

constexpr ImGuiWindowFlags CONTENT_PANEL_FLAGS = ImGuiViewportFlags_NoFocusOnAppearing;

void Hush::ContentPanel::Init(Scene *activeScene) noexcept
{
	activeScene->CreateQuery<ResourceManager, VirtualFilesystem>().Each([this](Entity& entt, ResourceManager& resourceManager, VirtualFilesystem& vfs) {
		resourceManager.Init(&vfs);
		this->m_resourceManager = &resourceManager;
		this->m_filesystem = &vfs;
	});
	this->m_folderImage = this->m_resourceManager->LoadTexture("engine_res://folder.png");
	this->m_fileImage = this->m_resourceManager->LoadTexture("engine_res://file.png");
}

void Hush::ContentPanel::OnRender() {
	if (ImGui::Begin("Project", nullptr, CONTENT_PANEL_FLAGS))
	{
		ImVec2 regionDimensions = ImGui::GetContentRegionAvail();
		
        float regionWidth = regionDimensions.x;

        ImGuiStyle& style = ImGui::GetStyle();
        const ImGuiContext* context = ImGui::GetCurrentContext();
        const ImGuiWindow* windowUnderMouse = context->CurrentWindow;
        float spacing = style.ItemSpacing.x;
        float cursorX = 0.0F;
        
		if (this->m_dirty) {
			this->RefreshDirectory();
			this->GenerateMetaFiles();
		}
		ImGui::Text("Current Working Directory: %s", this->m_currentWorkingDirectory.c_str());
		bool isMouseInScene = !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
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
                // Handle click if needed
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
		const ImGuiPayload *payload = ImGui::GetDragDropPayload();
		if (isMouseInScene && payload != nullptr && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			const auto* data = reinterpret_cast<const FileInfo*>(payload->Data);
			if (CanBeDroppedToScene(*data)) {
				LogFormat(ELogLevel::Info, "Dropped payload {}!", data->path.filename().string());
				IRenderer* renderer = WindowManager::GetMainWindow()->GetInternalRenderer();
				renderer->PushMesh(data->path.generic_string());
			}
		}
	}
	ImGui::End();
}


bool Hush::ContentPanel::CanBeDroppedToScene(const FileInfo& fileData) const {
	return fileData.flags == EFileFlags::File;
}

void Hush::ContentPanel::RefreshDirectory() {
	this->m_currentItems.clear();
	this->m_currentItems = this->m_filesystem->ListPath(this->m_currentWorkingDirectory);
	this->m_dirty = false;
}


void Hush::ContentPanel::GenerateMetaFiles() {
	// Scan directories recursively and query the database indexing (NYI)
	for(const FileInfo& data : this->m_currentItems) {
		if (!data.ShouldGenerateMetaFile()) {
			continue;
		}

		FileMetadata metadata = {
			.metadataVersion = FileMetadata::VERSION,
			.id = Hashing::Fnv1a(data.path.string())
		};

		// Create inner resource files
		

		this->MakeMetaFile(data, metadata);
	}
}

void Hush::ContentPanel::MakeMetaFile(const FileInfo& fileData, const FileMetadata& metadata) {
	std::string metaPath = fileData.path.string().append(".meta");
	if (std::filesystem::exists(metaPath)) {
		return;
	}
	Result<std::unique_ptr<IFile>, IFile::EError> openRes = this->m_filesystem->OpenFile(metaPath, EFileOpenMode::Write);
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
	
}

