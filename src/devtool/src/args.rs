use crate::commands::build::BuildCommand;
use crate::commands::clicommand::CliCommand;
use crate::commands::configure::ConfigureCommand;
use crate::commands::new_file::NewFileCommand;
use clap::{Parser, Subcommand};
use std::process::ExitCode;

#[derive(Parser)]
#[command(version, about)]
pub struct DevtoolCliOptions {
    #[clap(subcommand)]
    pub cmd: Cmd,
}

#[derive(Subcommand)]
pub enum Cmd {
    /// Configure the engine project
    Configure(ConfigureCommand),
    Build(BuildCommand),
    NewFile(NewFileCommand),
}

impl DevtoolCliOptions {
    pub fn execute(self) -> anyhow::Result<ExitCode> {
        match self.cmd {
            Cmd::Configure(config) => config.run(),
            Cmd::Build(build) => build.run(),
            Cmd::NewFile(new_file) => new_file.run(),
        }
    }
}
