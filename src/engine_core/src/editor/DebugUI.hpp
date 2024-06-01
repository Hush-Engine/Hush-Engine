/*! \file DebugUI.hpp
    \author Kyn21kx
    \date 2024-05-31
    \brief Shows debug options to the UI
*/

#pragma once
#include "IEditorPanel.hpp"
#include <imgui/imgui.h>
#include <log/Logger.hpp>
#include <scripting/DotnetHost.hpp>
#include <scripting/ScriptingManager.hpp>


#if defined(_WIN32)
constexpr std::string_view DOTNET_PATH = "C:/Program Files/dotnet/";
#elif defined(__APPLE__)
constexpr std::string_view DOTNET_PATH = "/usr/local/share/dotnet";
#else
constexpr std::string_view DOTNET_PATH = "/usr/share/dotnet";
#endif

constexpr std::string_view ASSEMBLY_TEST = "assembly-test";

constexpr uint32_t BUFF_SIZE = 100;

namespace Hush
{
    class DebugUI final : public IEditorPanel
    {
      public:
        
        DebugUI()
        {
            this->m_dotnetHost = std::make_shared<DotnetHost>(DOTNET_PATH.data());
            this->m_manager = std::make_unique<ScriptingManager>(this->m_dotnetHost, ASSEMBLY_TEST);
            this->m_manager->InvokeCSharp("Test", "Class1", "GetDotNetVersion");
            this->m_name = new char[BUFF_SIZE];
            this->m_name[0] = 0;
            this->m_className = new char[BUFF_SIZE];
            this->m_className[0] = 0;
            this->m_hashOutput = new char[BUFF_SIZE];
            this->m_hashOutput[0] = 0;
            this->m_toHash = new char[BUFF_SIZE];
            this->m_toHash[0] = 0;
            this->m_methodName = new char[BUFF_SIZE];
            this->m_methodName[0] = 0;
        }

        ~DebugUI()
        {
            delete this->m_hashOutput;
            delete this->m_name;
            delete this->m_className;
            delete this->m_hashOutput;
            delete this->m_toHash;
            delete this->m_methodName;
        }

        void OnRender() override
        {
            if (!ImGui::Begin("Debug options"))
            {
                return;
            }

            ImGui::Checkbox("Show Mouse Input", &S_SHOW_MOUSE_INFO);
            ImGui::Checkbox("Show Keyboard Input", &S_SHOW_KEYBOARD_INFO);
            if (ImGui::Button("Clear Logs"))
            {
                ClearLogs();
            }

            ImGui::Text("Select a class from C#");
            ImGui::InputText("class", this->m_className, BUFF_SIZE);
            ImGui::Text("Select a method from that class");
            ImGui::InputText("method", this->m_methodName, BUFF_SIZE);
            if (ImGui::Button("Script away! (void)"))
            {
                this->m_manager->InvokeCSharp("Test", this->m_className, this->m_methodName);
            }

            ImGui::Text("Input your name");
            ImGui::InputText("name", this->m_name, BUFF_SIZE);
            if (ImGui::Button("Greet me from C#\n(string demonstration)"))
            {
                this->m_manager->InvokeCSharp("Test", "Class1", "SayName", this->m_name);
            }

            ImGui::InputText("Get hash of...", this->m_toHash, BUFF_SIZE);
            if (ImGui::Button("Hash your input and display it on ImGui\n (returning demonstration)"))
            {
                this->m_hashOutput = this->m_manager->InvokeCSharpWithReturn<char *>(
                    "Test", "Class1", "CalculateHash", this->m_toHash);
            }

            if (this->m_hashOutput != nullptr && strlen(this->m_hashOutput) > 0)
            {
                ImGui::Text("Input (from C++):\n%s\nHashed output (from C#):\n%s", this->m_toHash, this->m_hashOutput);
            }

            ImGui::End();
        }

        static inline bool MouseInfoEnabled() {
            return S_SHOW_MOUSE_INFO;
        }
        static inline bool KeyboardInfoEnabled() {
            return S_SHOW_KEYBOARD_INFO;
        }

      private:
        static inline bool S_SHOW_MOUSE_INFO = false;
        static inline bool S_SHOW_KEYBOARD_INFO = false;
        std::shared_ptr<DotnetHost> m_dotnetHost = nullptr;
        std::unique_ptr<ScriptingManager> m_manager;
        char* m_hashOutput;
        char* m_className;
        char* m_methodName;
        char* m_name;
        char* m_toHash;
    };
}