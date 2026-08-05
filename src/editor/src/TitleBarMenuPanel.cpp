#include "TitleBarMenuPanel.hpp"
#include "Assertions.hpp"
#include "HushEngine.hpp"
#include "VirtualFilesystem.hpp"
#include "imguifiledialog/ImGuiFileDialog.h"
#include "networking/NetworkUtils.hpp"
#include <imgui/imgui.h>
#include "UI.hpp"

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

void Hush::TitleBarMenuPanel::FileMenuOptions()
{
	IGFD::FileDialog *fileDialog = ImGuiFileDialog::Instance();

	if (fileDialog->Display("SceneSave", ImGuiWindowFlags_NoCollapse, {800, 600})) {
		if (fileDialog->IsOk()) {
			// Save the scene

		}
		fileDialog->Close();
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
	}
	if (ImGui::MenuItem("Save", "Ctrl+S"))
	{
		// NYI: Must check if this scene is saved, otherwise go to save scene as
	}
	if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
	{
		// BACKLOG: Use our own file dialog instead of ImGui's
		IGFD::FileDialogConfig config;

		VirtualFilesystem* vfs = this->m_activeScene->GetEngine()->GetVirtualFilesystem();
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
