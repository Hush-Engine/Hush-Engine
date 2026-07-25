#include "ContentPanel.hpp"
#include "Components/GlobalKeys.hpp"
#include "Components/Material3D.hpp"
#include "Entity.hpp"
#include "FileMetadata.hpp"
#include "IFile.hpp"
#include "Query.hpp"
#include "RHI/IGraphicsDevice.hpp"
#include "Result.hpp"
#include "UI.hpp"
#include "VirtualFilesystem.hpp"
#include "WindowRenderer.hpp"
#include "components/EditorInfo.hpp"
#include "Loaders/GltfLoader.hpp"
#include "crypto/Hashing.hpp"
#include "Assertions.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include "serialization/Serialization.hpp"
#include <cstddef>
#include <filesystem>
#include <imgui/imgui.h>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <span>
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
		if (this->m_dirty)
		{
			this->RefreshDirectory();
			// this->GenerateMetaFiles();
			UI::S_INITIALIZED = true;
		}
		ImGui::Text("Current Working Directory: %s", this->m_currentWorkingDirectory.c_str());

		bool isMouseInScene = this->m_editorInfoRef.GetData<EditorInfo>()->isMouseOnScene;
		this->DrawFiles(isMouseInScene);

		const ImGuiPayload *payload = ImGui::GetDragDropPayload();
		if (isMouseInScene && payload != nullptr && payload->DataSize == sizeof(FileInfo) &&
			ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			const auto *data = reinterpret_cast<const FileInfo *>(payload->Data);
			if (CanBeDroppedToScene(*data))
			{
				Entity renderingSystemEnt = this->m_scene->CreateEntityWithKey("RenderingSystem");
				auto *systemRef = *renderingSystemEnt.GetComponent<RenderingSystem *>();

				HushEngine *engine = this->m_scene->GetEngine();
				Graphics::IGraphicsDevice *device = engine->GetWindowRenderer()->GetGraphicsDevice();
				RenderingContext ctx = {.materialDescriptor = &systemRef->GetPBRDescriptor(),
										.activeScene = this->m_scene,
										.resourceManager = this->m_resourceManager,
										.device = device};

				Entity rootEntity = GLTFLoader::GenerateMeshEntities(ctx, data->path);
				HUSH_ASSERT(rootEntity.IsValid(), "Failed to create meshes from the GLTF file!");
			}
		}
	}
	ImGui::End();
}

void Hush::ContentPanel::DrawFiles(bool isMouseInScene)
{
	ImVec2 regionDimensions = ImGui::GetContentRegionAvail();

	float regionWidth = regionDimensions.x;

	ImGuiStyle &style = ImGui::GetStyle();
	float spacing = style.ItemSpacing.x;
	float cursorX = 0.0F;
	for (const FileInfo &item : this->m_currentItems)
	{
		// TODO: Fix all the copies that this makes
		const std::string &fileName = item.path.filename().string();
		ImVec2 textSize = ImGui::CalcTextSize(fileName.c_str());
		float buttonWidth = textSize.x + style.FramePadding.x * 2.0F;

		// If this button would exceed the region width, wrap to next line
		if (cursorX + buttonWidth > regionWidth)
		{
			ImGui::NewLine();
			cursorX = 0.0F;
		}

		// Draw the button as a draggable source
		ImGui::PushID(fileName.c_str());
		if (ImGui::Button(fileName.c_str(), ImVec2(buttonWidth, 50.0F)))
		{
			// TODO: Handle the click stuff
		}
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
		ImGui::PopID();

		// Advance cursor and prepare for next same-line
		cursorX += buttonWidth + spacing;
		ImGui::SameLine(0.0F, spacing);
	}
}

bool Hush::ContentPanel::CanBeDroppedToScene(const FileInfo &fileData) const
{
	return fileData.flags == EFileFlags::File && fileData.IsModelFile();
}

void Hush::ContentPanel::RefreshDirectory()
{
	this->m_currentItems.clear();
	this->m_currentItems = this->m_filesystem->ListPath(this->m_currentWorkingDirectory);
	this->m_dirty = false;
}

void Hush::ContentPanel::GenerateMetaFiles()
{
	// Scan directories recursively and query the database indexing (NYI)
	for (const FileInfo &data : this->m_currentItems)
	{
		if (!data.ShouldGenerateMetaFile() || data.extension == EFileExtension::META)
		{
			continue;
		}

		FileMetadata metadata = {.metadataVersion = FileMetadata::VERSION, .id = Hashing::Fnv1a(data.path.string())};

		// Create inner resource files
		this->CreateInnerResources(data, metadata);

		// this->MakeMetaFile(data, metadata);
	}
}

void Hush::ContentPanel::MakeMetaFile(const FileInfo &fileData, const FileMetadata &metadata)
{
	std::filesystem::path metaPath = fileData.path.parent_path() / fileData.path.stem().string().append(".meta");
	if (std::filesystem::exists(metaPath))
	{
		return;
	}
	Result<std::unique_ptr<IFile>, IFile::EError> openRes =
		this->m_filesystem->OpenFile(metaPath.string(), EFileOpenMode::Write);
	HUSH_RESULT_ASSERT(openRes, "Failed to create metadata file!");

	std::unique_ptr<IFile> &file = openRes.value();
	Result<std::string, Serialization::ESerializationError> serializationResult =
		Serialization::SerializeJson(metadata);
	HUSH_RESULT_ASSERT(serializationResult, "Could not serialize metadata for file {}", fileData.path.string());

	std::string &json = serializationResult.value();
	std::span<const std::byte> writeBuffer(reinterpret_cast<const std::byte *>(json.data()), json.size());
	Result<void, IFile::EError> writeRes = file->Write(writeBuffer);
	HUSH_RESULT_ASSERT(writeRes, "Failed to write metadata file");

	file->Close();
}

void Hush::ContentPanel::CreateInnerResources(const FileInfo &fileData, [[maybe_unused]] const FileMetadata &metadata)
{
	switch (fileData.extension)
	{
	default:
		break;
	case EFileExtension::GLB:
		// fastgltf::Expected<fastgltf::Asset> asset = GltfLoadFunctions::GetAssetFromFile(fileData.path);
		// HUSH_ASSERT(asset, "GLTF asset at {} not properly loaded, error: {}!", fileData.path.string(),
		// 			fastgltf::getErrorMessage(asset.error()));
		// size_t cntr = 0;
		// TODO: Swap for regular for loop
		// for (const fastgltf::Image &image : asset->images)
		{
			// Write the binary data to the png

			// fastgltf::MimeType mimeType = fastgltf::MimeType::None;
			// const std::span<const std::byte> imageBuffer =
			// 	GltfLoadFunctions::ExtractImageBuffer(image, asset.get(), &mimeType);
			// if (imageBuffer.empty())
			// {
			// 	continue;
			// }
			// std::filesystem::path parentDir = fileData.path.parent_path();
			// std::string textName;
			// if (image.name.empty())
			// {
			// 	// I know, I know
			// 	textName = fileData.path.stem()
			// 				   .string()
			// 				   .append("_")
			// 				   .append(std::to_string(cntr))
			// 				   .append(".")
			// 				   .append(magic_enum::enum_name(mimeType));
			// }
			// else
			// {
			// 	textName = image.name;
			// }
			// std::filesystem::path generatedFileName = parentDir / textName;
			// Result<std::unique_ptr<IFile>, IFile::EError> createFileRes =
			// 	this->m_filesystem->OpenFile(generatedFileName.string(), EFileOpenMode::Write);
			// HUSH_RESULT_ASSERT(createFileRes, "Failed to create inner resource for asset {}, on resource {}",
			// 				   fileData.path, image.name);
			// std::unique_ptr<IFile> &createdFile = createFileRes.value();
			// Result<void, IFile::EError> writeResult = createdFile->Write(imageBuffer);
			// HUSH_RESULT_ASSERT(writeResult, "Failed to write image buffer");
			// createdFile->Close();

			// FileMetadata metadata = {.metadataVersion = FileMetadata::VERSION,
			// 						 .id = Hashing::Fnv1a(createdFile->GetFileInfo().path.string())};
			// this->MakeMetaFile(createdFile->GetFileInfo(), metadata);
			// this->m_currentItems.emplace_back(createdFile->GetFileInfo());
			// cntr++;
		}
		break;
	}
}
