use anyhow::anyhow;
use regex::Regex;
use std::process::Command;

/// Gets the installed macOS version
pub fn cmake_version() -> anyhow::Result<String> {
    let re = Regex::new(r"version\s+(\d+\.\d+\.\d+)")?;

    let cmake_command = Command::new("cmake").arg("--version").output()?;

    let cmake_version = String::from_utf8_lossy(&cmake_command.stdout);

    let extracted_version = re
        .find(&cmake_version)
        .ok_or_else(|| anyhow!("Cannot find CMake command"))?;

    Ok(extracted_version.as_str().to_string())
}
