# Filename: bootstrap.ps1
# Author: Alan Ramirez Herrera
# Date: 2024-02-27
# Brief: Sets the PATH to use devtool

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$VcpkDir = Join-Path -Path $ProjectRoot -ChildPath 'vcpkg'

$env:PATH = $env:PATH + ";$ScriptDir" + ";$VcpkDir"
$env:VCPKG_ROOT = $VcpkDir