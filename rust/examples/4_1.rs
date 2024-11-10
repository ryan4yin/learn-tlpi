use clap::Parser;
use nix::{
    fcntl::{open, OFlag},
    sys::stat::Mode,
    unistd::{read, write},
};
use std::os::fd::{AsRawFd, FromRawFd, OwnedFd};
use std::path::Path;

const MAX_BYTES: usize = 4096;

// Copy standard input to each FILE, and also to standard output.
#[derive(Parser)] // requires `derive` feature
#[command(version, about, long_about = None)]
struct Cli {
    // append to the given FILEs, do not overwrite
    #[arg(short = 'a')]
    append: bool,

    // ignore interrupt signals
    #[arg(short = 'i')]
    ignore_interrupts: bool,

    // operate in a more appropriate MODE with pipes.
    #[arg(short = 'p')]
    p: bool,

    file: Vec<String>,
}

fn main() {
    let args = Cli::parse();
    let flag = if args.append {
        OFlag::O_CREAT | OFlag::O_WRONLY | OFlag::O_APPEND
    } else {
        OFlag::O_CREAT | OFlag::O_WRONLY | OFlag::O_TRUNC
    };
    let mode = Mode::from_bits(0o666).unwrap();
    let files: Vec<OwnedFd> = args
        .file
        .iter()
        .map(|name| {
            let path = Path::new(name);
            let raw_fd = open(path, flag, mode).unwrap();
            // This closes the file descriptor on drop.
            // So we do not need to close it manually.
            //
            // https://github.com/nix-rust/nix/issues/1750
            unsafe { OwnedFd::from_raw_fd(raw_fd) }
        })
        .collect();

    let mut buffer = [0u8; MAX_BYTES];
    let stdin = std::io::stdin().as_raw_fd();
    let mut n;
    loop {
        n = read(stdin, &mut buffer).unwrap();
        if n == 0 {
            break;
        }

        for file in &files {
            let w_n = write(file, &buffer[..n]).unwrap();
            if w_n != n {
                panic!("write error");
            }
        }
    }
}
