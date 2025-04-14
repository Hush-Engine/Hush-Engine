
command = '-external:ID:\\hush\\Hush-Engine\\third_party\\coreclr\\include -external:ID:\\hush\\Hush-Engine\\build\\windows-x64-debug\\vcpkg_installed\\x64-windows\\include -external:ID:\\hush\\Hush-Engine\\third_party\\zadeh\\include -external:ID:\\hush\\Hush-Engine\\build\\windows-x64-debug\\vcpkg_installed\\x64-windows\\include\\SDL2 -external:ID:\\hush\\Hush-Engine\\third_party\\stb\\include -external:ID:\\hush\\Hush-Engine\\build\\windows-x64-debug\\vcpkg_installed\\x64-windows\\include\\spirv-reflect'

print(command.replace('-external:ID', '-I'))