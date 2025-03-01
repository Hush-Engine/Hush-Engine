use anyhow::anyhow;
use regex::Regex;
use std::process::{Command, Stdio};
use tracing::error;

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

pub fn git_username() -> anyhow::Result<String> {
    let git_command = Command::new("git")
        .arg("config")
        .arg("user.name")
        .output()?;

    let user_name = String::from_utf8_lossy(&git_command.stdout);

    Ok(user_name.trim().to_string())
}

pub fn get_project_files(filter: Vec<&str>, only_modified: bool) -> anyhow::Result<Vec<String>> {
    let mut command = Command::new("git");
    command
        .args(["ls-files"])
        .args(filter)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped());

    if only_modified {
        command.arg("-m");
    }

    let output = command.output()?;
    if !output.status.success() {
        let error_message = "git ls-files -m failed";
        error!("Output: {}", String::from_utf8_lossy(&output.stderr));
        return Err(anyhow!(error_message));
    }

    let modified_files = String::from_utf8_lossy(&output.stdout)
        .lines()
        .map(|s| s.to_string())
        .collect();

    Ok(modified_files)
}

pub fn get_diff_files(filter: Vec<&str>, branch: &str) -> anyhow::Result<Vec<String>> {
    let output = Command::new("git")
        .args(["diff", "--name-only", branch])
        .args(filter)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .output()?;

    if !output.status.success() {
        let error_message = "git diff --name-only failed";
        error!("Output: {}", String::from_utf8_lossy(&output.stderr));
        return Err(anyhow!(error_message));
    }

    let modified_files = String::from_utf8_lossy(&output.stdout)
        .lines()
        .map(|s| s.to_string())
        .collect();

    Ok(modified_files)
}

pub fn create_stdio(verbose: bool) -> (Stdio, Stdio) {
    if verbose {
        (Stdio::inherit(), Stdio::inherit())
    } else {
        (Stdio::piped(), Stdio::piped())
    }
}
