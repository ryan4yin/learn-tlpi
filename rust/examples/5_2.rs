use nix::{
    fcntl::{open, OFlag},
    sys::stat::Mode,
    unistd::{lseek, write, Whence},
};
use std::os::fd::{AsRawFd, FromRawFd, OwnedFd};
use std::path::Path;

fn main() {
    let fd = {
        let path = Path::new("append_and_lseek_start.txt");
        let flag = OFlag::O_WRONLY | OFlag::O_APPEND;
        let mode = Mode::from_bits(0o750).unwrap();
        let raw_fd = open(path, flag, mode).expect("failed to open file");
        unsafe { OwnedFd::from_raw_fd(raw_fd) }
    };
    lseek(fd.as_raw_fd(), 0, Whence::SeekSet).expect("failed to seek");
    write(&fd, "xxxxxxxxxx".as_bytes()).expect("failed to write '1' to file");
}
