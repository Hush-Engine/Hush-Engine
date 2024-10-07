use std::process::ExitCode;
use crate::commands::configure::ConfigureCommand;
use clap::{Parser, Subcommand};
use crate::commands::build::BuildCommand;
use crate::commands::clicommand::CliCommand;

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
}

impl DevtoolCliOptions {
    pub fn execute(self) -> anyhow::Result<ExitCode> {
        match self.cmd {
            Cmd::Configure(config) => config.run(),
            Cmd::Build(build) => build.run(),
        }
    }
}