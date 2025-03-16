use crate::commands::clicommand::CliCommand;
use clap::Parser;
use std::process::ExitCode;

/// Build command
#[derive(Debug, Parser)]
pub struct ShaderCompileCommand {
    /// Verbose output
    #[arg(default_value_t = false, short, long)]
    verbose: bool,
}

impl CliCommand for ShaderCompileCommand {
    fn run(self) -> anyhow::Result<ExitCode> {
        let glslang_version_cmd = std::process::Command::new("glslang")
            .arg("--version")
            .output()?;
        if !glslang_version_cmd.status.success() {
            let error = String::from_utf8_lossy(&glslang_version_cmd.stdout);
            let extra_string = if self.verbose {
                String::new()
            } else {
                format!("\n{}", error)
            };
            tracing::error!("Build Failed{}", extra_string);
            return Ok(ExitCode::FAILURE);
        }

        // Get all the .frag and .vert files
        let filter = vec!["frag", "vert"];
        let resources = std::fs::read_dir("./res/")
            .unwrap()
            .filter_map(|f| f.ok())
            .filter(|file_entry| {
                filter
                    .iter()
                    .any(|&ext| file_entry.path().extension().unwrap() == ext)
            })
            .map(|entry| {
                let path = entry.path();
                return format!("{} {} {}.spv", path.display(), "-V -o", path.display());
            })
            .collect::<Vec<_>>();

        let compile_command = std::process::Command::new("glslang")
            .args(resources)
            .output()?;

        Ok(if compile_command.status.success() {
            ExitCode::SUCCESS
        } else {
            ExitCode::FAILURE
        })
    }
}
