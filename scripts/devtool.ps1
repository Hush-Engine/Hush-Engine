# Filename: devtool.ps1
# Author: Alan Ramirez Herrera
# Date: 2023-12-12
# Brief: Wrapper around devtool

$ScriptDir = Join-Path -Path $PSScriptRoot -ChildPath 'devtool.py'

python $ScriptDir  $args
