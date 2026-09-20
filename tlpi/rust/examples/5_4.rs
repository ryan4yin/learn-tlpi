use core::str;
use nix::{
    fcntl::{self, open, OFlag},
    sys::stat::Mode,
    unistd::{close, lseek, read, write, Whence},
};
use std::os::fd::{FromRawFd, IntoRawFd, OwnedFd, RawFd};
use std::path::Path;

const MAX_BYTES: usize = 20;

fn dup2(oldfd: RawFd, newfd: RawFd) -> nix::Result<RawFd> {
    // test if oldfd exist
    fcntl::fcntl(oldfd, fcntl::F_GETFL)?;

    if oldfd == newfd {
        return Ok(oldfd);
    }

    if fcntl::fcntl(newfd, fcntl::F_GETFL).is_ok() {
        // newfd already opened, close it first
        close(oldfd)?;
    }

    // copy oldfd
    fcntl::fcntl(oldfd, fcntl::F_DUPFD(newfd))
}

fn main() {
    let args = std::env::args().collect::<Vec<String>>();

    if args.len() != 2 {
        panic!("should pass one & only one args!")
    }

    let oldfd = {
        let path = Path::new(&args[1]);
        let flag = OFlag::O_RDWR;
        let mode = Mode::from_bits(0o700).unwrap();
        open(path, flag, mode).expect("failed to open file")
    };

    // try to use 10 as newfd's number
    let newfd = 10.into_raw_fd();
    let dupfd = dup2(oldfd, newfd).unwrap();
    let newfd_owned = unsafe { OwnedFd::from_raw_fd(newfd) };
    println!("newfd: {}, dupfd: {}", newfd, dupfd);

    // read by oldfd
    let mut buffer = [0u8; MAX_BYTES];
    let str1 = {
        read(oldfd, &mut buffer).unwrap();
        str::from_utf8(&buffer).unwrap()
    };
    println!("the string just readed: {}", str1);

    // check newfd's flag
    let flags = fcntl::fcntl(newfd, fcntl::F_GETFL).unwrap();

    println!(
        "newfd's flags contains O_RDWR: {}",
        flags & nix::libc::O_RDWR != 0
    );
    println!(
        "newfd's flags contains O_APPEND: {}",
        flags & nix::libc::O_APPEND != 0
    );

    // add O_WRITE flags into oldfd
    let buf2 = "--write before add O_APPEND flag".as_bytes();
    lseek(newfd, 0, Whence::SeekSet).expect("failed to seek to start");
    write(&newfd_owned, buf2).unwrap();

    // =================================================

    println!("now we change oldfd's flag O_RDWR => O_RDWR | O_APPEND!");
    fcntl::fcntl(oldfd, fcntl::F_SETFL(OFlag::O_RDWR | OFlag::O_APPEND)).unwrap();

    // =================================================

    // check newfd's flags again
    let flags = fcntl::fcntl(newfd, fcntl::F_GETFL).unwrap();

    println!(
        "newfd's flags contains O_RDWR: {}",
        flags & nix::libc::O_RDWR != 0
    );
    println!(
        "newfd's flags contains O_APPEND: {}",
        flags & nix::libc::O_APPEND != 0
    );

    // try to write some text into newfd again.
    let buf2 = "---write after add O_APPEND flag".as_bytes();
    lseek(newfd, 0, Whence::SeekSet).expect("failed to seek to start");
    write(&newfd_owned, buf2).unwrap();
}
