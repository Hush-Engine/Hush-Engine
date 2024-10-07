use std::process::ExitCode;
use crate::args::{DevtoolCliOptions};
use clap::Parser;
use tracing::Level;
use tracing_subscriber::FmtSubscriber;

mod args;
mod commands;

fn main() -> anyhow::Result<ExitCode> {
    let subscriber = FmtSubscriber::builder()
        .compact()
        .with_file(false)
        .with_line_number(false)
        .without_time()
        .with_target(false)
        // all spans/events with a level higher than TRACE (e.g, debug, info, warn, etc.)
        // will be written to stdout.
        .with_max_level(Level::INFO)
        // completes the builder.
        .finish();

    tracing::subscriber::set_global_default(subscriber)
        .expect("setting default subscriber failed");

    let cli = DevtoolCliOptions::parse();

    cli.execute()
}
