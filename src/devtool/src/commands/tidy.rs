///  Filename: tidy.rs
/// Author: Alan Ramirez
/// Date: 2024-12-25
/// Brief: Clang tidy command
use crate::commands::clicommand::CliCommand;
use crate::commands::utils::{create_stdio, get_project_files};
use anyhow::anyhow;
use clap::Parser;
use std::path::Path;
use std::process::{Command, ExitCode};
use tracing::{error, info};

/// Format command
#[derive(Debug, Parser)]
pub struct TidyCommand {
    /// Verbose output
    #[arg(default_value_t = false, short, long)]
    verbose: bool,

    /// Run clang-tidy on all files. This takes a long time.
    #[arg(default_value_t = false, short, long)]
    all_files: bool,

    /// Fix issues automatically
    #[arg(default_value_t = false, short, long)]
    fix: bool,

    /// Build folder
    /// This is the folder where the build files are located
    #[arg(default_value = "build", short, long)]
    build_folder: String,
}

impl TidyCommand {
    fn clang_tidy(&self) -> anyhow::Result<()> {
        let (stdout_output, stderr_output) = create_stdio(self.verbose);

        info!("Running clang-tidy on project files");

        let files = get_project_files(vec!["*.c", "*.cpp", "*.h", "*.hpp"], !self.all_files)?
            .into_iter()
            .filter(|f| !f.starts_with("third_party"))
            // Filter existing files
            .filter(|f| {
                let path = Path::new(f);
                path.exists() && path.is_file()
            })
            .collect::<Vec<_>>();

        if files.is_empty() {
            info!("No files to run clang-tidy on");
            return Ok(());
        }

        let command = Command::new("clang-tidy")
            .args([if self.fix { "--fix --fix-errors" } else { "" }])
            .args(["-p", &self.build_folder])
            .args(["--header-filter=src/*"])
            .arg("--quiet")
            .args(["--config-file=.clang-tidy"])
            .args(files)
            .stdout(stdout_output)
            .stderr(stderr_output)
            .spawn()?;

        let output = command.wait_with_output();

        if let Err(e) = &output {
            match e.kind() {
                std::io::ErrorKind::NotFound => {
                    error!("clang-tidy not found. Please install clang-tidy");
                }
                _ => {
                    error!("Error running clang-tidy: {}", e);
                }
            }
        }

        if let Ok(status) = output {
            if !status.status.success() {
                let error_message = String::from_utf8_lossy(&status.stdout);
                error!("{}", error_message);
                return Err(anyhow!("clang-tidy failed"));
            }

            info!("clang-tidy finished");
        }

        Ok(())
    }
}

impl CliCommand for TidyCommand {
    fn run(self) -> anyhow::Result<ExitCode> {
        self.clang_tidy()?;

        Ok(ExitCode::SUCCESS)
    }
}
