use crate::commands::clicommand::CliCommand;
use clap::Parser;
use lazy_static::lazy_static;
use rfd::FileDialog;
use std::collections::HashMap;
use std::ffi::OsStr;
use std::fs::File;
use std::io::Write;
use std::path::{Path, PathBuf};
use std::process::ExitCode;
use tracing::{error, info};

/// Build command
#[derive(Debug, Parser)]
pub struct NewFileCommand {
    /// File path
    file_path: Option<PathBuf>,

    /// File brief
    brief: Option<String>,
}

const CMAKE_FILE: &str = include_str!("../../templates/.cmake");
const CPP_FILE: &str = include_str!("../../templates/.cpp");
const CSHARP_FILE: &str = include_str!("../../templates/.cs");
const HPP_FILE: &str = include_str!("../../templates/.hpp");
const PS1_FILE: &str = include_str!("../../templates/.ps1");
const PY_FILE: &str = include_str!("../../templates/.py");
const SH_FILE: &str = include_str!("../../templates/.sh");

lazy_static! {
    static ref TEMPLATES: HashMap<&'static str, &'static str> = {
        let mut m = HashMap::new();
        m.insert(".cmake", CMAKE_FILE);
        m.insert(".cpp", CPP_FILE);
        m.insert(".cs", CSHARP_FILE);
        m.insert(".hpp", HPP_FILE);
        m.insert(".ps1", PS1_FILE);
        m.insert(".py", PY_FILE);
        m.insert(".sh", SH_FILE);

        m
    };
}

impl CliCommand for NewFileCommand {
    fn run(self) -> anyhow::Result<ExitCode> {
        let new_file_path = self
            .file_path
            .unwrap_or_else(|| FileDialog::new().pick_file().unwrap_or(PathBuf::new()));
        let brief = self.brief.unwrap_or_else(|| "".into());

        // TODO: finish

        let extension = new_file_path
            .extension()
            .map(|p| p.to_str())
            .flatten()
            .unwrap_or("");

        let template = TEMPLATES.get(extension);

        if new_file_path.extension().is_none() || template.is_none() {
            // Create it as-is
            File::create(new_file_path)?;

            info!("File created successfully");

            Ok(ExitCode::SUCCESS)
        } else if let Some(template) = template {
            let mut file = File::create(template)?;
            file.write_all(template.as_bytes())?;

            info!("File created from template successfully");

            Ok(ExitCode::SUCCESS)
        } else {
            error!("Could not create file");
            Ok(ExitCode::FAILURE)
        }
    }
}
