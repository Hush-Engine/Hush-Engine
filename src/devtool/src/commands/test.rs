use crate::commands::clicommand::CliCommand;
use crate::commands::utils::check_cmake_version;
use anyhow::anyhow;
use clap::Parser;
use std::process::{ExitCode, Stdio};
use std::time::Instant;

/// Run CTest for a preset
#[derive(Debug, Parser)]
pub struct TestCommand {
    /// Preset to use, for example, windows-x64-debug
    #[arg(short, long)]
    preset: String,

    /// Verbose output
    #[arg(default_value_t = false, short, long)]
    verbose: bool,

    /// Test target regex
    #[arg(short, long)]
    target: Option<String>,
}

impl CliCommand for TestCommand {
    fn run(self) -> anyhow::Result<ExitCode> {
        if !check_cmake_version(3, 26)? {
            return Err(anyhow!("CMake must be version 3.26 or newer"));
        }

        let stdout_output = if self.verbose {
            Stdio::inherit()
        } else {
            Stdio::piped()
        };
        let stderr_output = if self.verbose {
            Stdio::inherit()
        } else {
            Stdio::piped()
        };

        let start = Instant::now();

        let mut ctest_command = std::process::Command::new("ctest");

        ctest_command
            .arg("--test-dir")
            .arg(format!("build/{}", self.preset))
            .arg("--output-on-failure");

        if let Some(target) = &self.target {
            ctest_command.arg(format!("--tests-regex={target}"));
        }

        let ctest_command = ctest_command
            .stdout(stdout_output)
            .stderr(stderr_output)
            .stdin(Stdio::null())
            .output()?;

        if ctest_command.status.success() {
            let duration = start.elapsed();
            tracing::info!("Tests finished in {:.2}s", duration.as_secs_f32());
        } else {
            let error = String::from_utf8_lossy(&ctest_command.stdout);
            let extra_string = if self.verbose {
                String::new()
            } else {
                format!("\n{error}")
            };
            tracing::error!("Tests failed{}", extra_string);
        }

        Ok(if ctest_command.status.success() {
            ExitCode::SUCCESS
        } else {
            ExitCode::FAILURE
        })
    }
}
