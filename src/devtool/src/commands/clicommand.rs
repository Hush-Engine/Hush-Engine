use anyhow::Result;
use std::process::ExitCode;
pub trait CliCommand: Sized {
    /// Runs the command
    ///
    /// # Return
    /// True if the command ran successfully, false otherwise.
    fn run(self) -> Result<ExitCode>;
}
