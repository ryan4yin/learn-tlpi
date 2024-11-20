use clap::Parser;
use nix::{
    fcntl::{open, OFlag},
    sys::stat::Mode,
    unistd::{lseek, write, Whence},
};
use std::os::fd::{AsRawFd, FromRawFd, OwnedFd};
use std::path::Path;

// Copy standard input to each FILE, and also to standard output.
#[derive(Parser)] // requires `derive` feature
#[command(version, about, long_about = None)]
struct Cli {
    filename: String,
    num_bytes: u64,
    #[arg(
        value_parser = clap::builder::PossibleValuesParser::new(["x", ""]),
    )]
    x: Option<String>,
}

fn main() {
    let args = Cli::parse();

    let use_append_flag = !args.x.is_some_and(|x| x == "x");
    println!("use_append_flag: {}", use_append_flag);

    let fd = {
        let path = Path::new(&args.filename);
        let flag = if use_append_flag {
            OFlag::O_CREAT | OFlag::O_WRONLY | OFlag::O_APPEND
        } else {
            OFlag::O_CREAT | OFlag::O_WRONLY
        };
        let mode = Mode::from_bits(0o750).unwrap();
        let raw_fd = open(path, flag, mode).expect("failed to open file");
        unsafe { OwnedFd::from_raw_fd(raw_fd) }
    };
    for _ in 1..=args.num_bytes {
        write(&fd, "1".as_bytes()).expect("failed to write '1' to file");

        if !use_append_flag {
            lseek(fd.as_raw_fd(), 0, Whence::SeekEnd).expect("failed to seek");
        }
    }
}
