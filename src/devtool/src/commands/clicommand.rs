use std::process::ExitCode;
use anyhow::Result;
pub trait CliCommand : Sized {
    /// Runs the command
    ///
    /// # Return
    /// True if the command ran successfully, false otherwise.
    fn run(self) -> Result<ExitCode>;
}
