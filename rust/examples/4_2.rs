use nix::{
    fcntl::{open, OFlag},
    sys::stat::Mode,
    unistd::{lseek, read, write, Whence},
};
use std::os::fd::{AsRawFd, FromRawFd, OwnedFd};
use std::path::Path;

const MAX_BYTES: usize = 4096;

/*
# 文件大小为 10.5 MB
› ls file_with_hole.txt
╭───┬────────────────────┬──────┬─────────┬──────────────╮
│ # │        name        │ type │  size   │   modified   │
├───┼────────────────────┼──────┼─────────┼──────────────┤
│ 0 │ file_with_hole.txt │ file │ 10.5 MB │ a minute ago │
╰───┴────────────────────┴──────┴─────────┴──────────────╯

# 但是实际占用的磁盘空间只有 8 KB
› ^du -sh file_with_hole.txt
8.0K    file_with_hole.txt

# 使用 cp 与 rsync 分别复制文件
› cp file_with_hole.txt file_with_hole_cp.txt
› rsync file_with_hole.txt file_with_hole_rsync.txt

# 查看复制后的文件大小
› ls *.txt
╭───┬──────────────────────────┬──────┬─────────┬───────────────╮
│ # │           name           │ type │  size   │   modified    │
├───┼──────────────────────────┼──────┼─────────┼───────────────┤
│ 0 │ file_with_hole.txt       │ file │ 10.5 MB │ 4 minutes ago │
│ 1 │ file_with_hole_cp.txt    │ file │ 10.5 MB │ now           │
│ 2 │ file_with_hole_rsync.txt │ file │ 10.5 MB │ now           │
╰───┴──────────────────────────┴──────┴─────────┴───────────────╯

# 查看复制后的文件占用的磁盘空间
# cp 正确处理了文件空洞，而 rsync 则没有
› ^du -sh *.txt
8.0K    file_with_hole.txt
8.0K    file_with_hole_cp.txt
11M     file_with_hole_rsync.txt
*/
fn generate_file_with_hole() {
    let str1 = "it is a dream that will never come true...\n";
    let str2 = "\nbut never give up, always try to do something meaningful...\n";
    let index = 1024 * 1024 * 10; // 10 MiB

    let fd = {
        let path = Path::new("file_with_hole.txt");
        let flag = OFlag::O_CREAT | OFlag::O_WRONLY | OFlag::O_TRUNC;
        let mode = Mode::from_bits(0o666).unwrap();
        let raw_fd = open(path, flag, mode).expect("failed to open file");
        unsafe { OwnedFd::from_raw_fd(raw_fd) }
    };

    write(&fd, str1.as_bytes()).expect("failed to write str1 to file");
    lseek(fd.as_raw_fd(), index, Whence::SeekSet).expect("failed to seek");
    write(&fd, str2.as_bytes()).expect("failed to write str2 to file");
}

fn copy() {
    let args = std::env::args().skip(1).collect::<Vec<String>>();

    let in_fd = {
        let path = Path::new(args[0].as_str());
        let raw_fd = open(path, OFlag::O_RDONLY, Mode::S_IRUSR).expect("failed to open src file");
        // This closes the file descriptor on drop.
        // So we do not need to close it manually.
        //
        // https://github.com/nix-rust/nix/issues/1750
        unsafe { OwnedFd::from_raw_fd(raw_fd) }
    };

    let out_fd = {
        let out_flag = OFlag::O_CREAT | OFlag::O_WRONLY | OFlag::O_TRUNC;
        let out_mode = Mode::from_bits(0o666).unwrap();
        let path = Path::new(args[1].as_str());
        let raw_fd = open(path, out_flag, out_mode).expect("failed to open dst file");
        unsafe { OwnedFd::from_raw_fd(raw_fd) }
    };

    let mut seek_index: usize = 0;
    let mut buffer = [0u8; MAX_BYTES];
    loop {
        let n = read(in_fd.as_raw_fd(), &mut buffer).expect("failed to read from src file");
        if n == 0 {
            break;
        }

        let mut file_hole = true;
        let mut byte_start: usize = 0;
        for i in 0..n {
            if buffer[i] == 0 {
                // skip file holes
                continue;
            }

            if file_hole {
                // i is the start of a non-hole block, and an end of a hole block
                file_hole = false;
                byte_start = i;
            } else if buffer[i + 1] != 0 {
                // i is the middle of a non-hole block
                continue;
            } else {
                // i is the end of a non-hole block, and a start of a hole block
                file_hole = true;
                let block = &buffer[byte_start..=i];
                let index = seek_index + byte_start;
                // seek to the start of the block, and write the block to the dst file
                lseek(out_fd.as_raw_fd(), index as i64, Whence::SeekSet).expect("failed to seek");

                // we have to use brrowed fd here, because this is a for loop!
                // if we use owned fd, the fd will be moved to the closure, and we can't use it in the next iteration.
                write(&out_fd, block).expect("failed to write to dst file");
            }
        }

        seek_index += n;
    }
}

fn main() {
    // generate_file_with_hole();
    copy();
}
