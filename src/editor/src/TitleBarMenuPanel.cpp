#include "TitleBarMenuPanel.hpp"
#include "Assertions.hpp"
#include "HushEngine.hpp"
#include "Logger.hpp"
#include "Ref.hpp"
#include "ResourceManager.hpp"
#include "SceneAsset.hpp"
#include "UIUtils.hpp"
#include "VirtualFilesystem.hpp"
#include "imguifiledialog/ImGuiFileDialog.h"
#include "networking/NetworkUtils.hpp"
#include <cstdio>
#include <fstream>
#include <imgui/imgui.h>
#include <ios>
#include <magic_enum/magic_enum.hpp>
#include "UI.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include "serialization/Serialization.hpp"

constexpr ImGuiWindowFlags PANEL_FLAGS = ImGuiWindowFlags_MenuBar;

void Hush::TitleBarMenuPanel::Init(Scene *activeScene) noexcept
{
	this->m_activeScene = activeScene;
}

void Hush::TitleBarMenuPanel::OnRender([[maybe_unused]] float deltaTime) noexcept
{
	if (ImGui::BeginMainMenuBar())
	{
		FileMenuOptions();
		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("About Hush Engine"))
			{
				Networking::SystemOpenURL("https://hushengine.com/");
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	// TODO: Also render the play options here
}

void Hush::TitleBarMenuPanel::SaveSceneDialog(IGFD::FileDialog *fileDialog)
{
	if (fileDialog->IsOk())
	{
		// Create a scene asset
		ResourceManager *resourceManager = this->m_activeScene->GetEngine()->GetResourceManager();
		std::string path = fileDialog->GetFilePathName();
		Ref<SceneAsset> scene = resourceManager->AllocateRef<SceneAsset>(path);
		// Save the scene
		Scene::EError err = this->m_activeScene->ToSceneAsset(scene.Get());

		// Serialize asset and save it to a file
		// I hate std::fstreams
		{
			std::ofstream ostream{};
			ostream.open(path);
			ostream << scene->sceneJson;
			ostream.close();
		}
		Entity notificationEnt = this->m_activeScene->CreateEntity();
		if (err == Scene::EError::None)
		{
			notificationEnt.EmplaceComponent<ToastNotification>("Scene saved succesfully!", 2.f,
																ToastNotification::EToastType::Info);
		}
		else
		{
			std::string message = "Failed to save scene: ";
			message += magic_enum::enum_name(err);
			notificationEnt.EmplaceComponent<ToastNotification>(message, 2.f, ToastNotification::EToastType::Error);
		}
	}
	fileDialog->Close();
}

void Hush::TitleBarMenuPanel::LoadSceneDialog(IGFD::FileDialog *fileDialog)
{
	if (fileDialog->IsOk())
	{
		// Create a scene asset
		ResourceManager *resourceManager = this->m_activeScene->GetEngine()->GetResourceManager();
		std::string path = fileDialog->GetFilePathName();
		Ref<SceneAsset> scene = resourceManager->AllocateRef<SceneAsset>(path);
		// I hate std::fstreams
		{
			std::ifstream istream(path, std::ios::binary | std::ios::ate);
			std::streamsize size = istream.tellg();
			istream.seekg(0, std::ios::beg);

			scene->sceneJson.resize(size);
			istream.read(scene->sceneJson.data(), size);
			istream.close();
		}
		// Load the scene
		Scene::EError err = this->m_activeScene->FromSceneAsset(scene.Get());
		Entity notificationEnt = this->m_activeScene->CreateEntity();
		if (err == Scene::EError::None)
		{
			notificationEnt.EmplaceComponent<ToastNotification>("Scene loaded succesfully!", 2.f,
																ToastNotification::EToastType::Info);
		}
		else
		{
			std::string message = "Failed to load scene: ";
			message += magic_enum::enum_name(err);
			notificationEnt.EmplaceComponent<ToastNotification>(message, 2.f, ToastNotification::EToastType::Error);
		}
	}
	fileDialog->Close();
}

void Hush::TitleBarMenuPanel::FileMenuOptions()
{
	IGFD::FileDialog *fileDialog = ImGuiFileDialog::Instance();

	if (fileDialog->Display("SceneSave", ImGuiWindowFlags_NoCollapse, {800, 600}))
	{
		this->SaveSceneDialog(fileDialog);
	}
	else if (fileDialog->Display("LoadScene", ImGuiWindowFlags_NoCollapse, {800, 600}))
	{
		this->LoadSceneDialog(fileDialog);
	}

	if (!ImGui::BeginMenu("File"))
	{
		return;
	}

	if (ImGui::MenuItem("New Scene", "Ctrl+N"))
	{
	}
	if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
	{
		// BACKLOG: Use our own file dialog instead of ImGui's
		IGFD::FileDialogConfig config;

		VirtualFilesystem *vfs = this->m_activeScene->GetEngine()->GetVirtualFilesystem();
		auto pathResolveRes = vfs->ResolveVirtualPath("res://");
		HUSH_RESULT_ASSERT(pathResolveRes, "Could not resolve virtual filesystem! This should never happen");

		config.path = pathResolveRes.value();
		fileDialog->OpenDialog("LoadScene", "Load Scene...", ".hscene", config);
	}
	if (ImGui::MenuItem("Save", "Ctrl+S"))
	{
		// NYI: Must check if this scene is saved, otherwise go to save scene as
	}
	if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
	{
		// BACKLOG: Use our own file dialog instead of ImGui's
		IGFD::FileDialogConfig config;

		VirtualFilesystem *vfs = this->m_activeScene->GetEngine()->GetVirtualFilesystem();
		auto pathResolveRes = vfs->ResolveVirtualPath("res://");
		HUSH_RESULT_ASSERT(pathResolveRes, "Could not resolve virtual filesystem! This should never happen");

		config.path = pathResolveRes.value();
		fileDialog->OpenDialog("SceneSave", "Save Scene As...", ".hscene", config);
	}

	if (ImGui::BeginMenu("Settings"))
	{
		if (ImGui::MenuItem("Editor Settings"))
		{
		}
		if (ImGui::MenuItem("Project Settings"))
		{
		}
		ImGui::EndMenu();
	}

	ImGui::EndMenu();
}
