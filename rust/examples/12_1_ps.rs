// 输入用户名, 输出该用户下的所有进程信息
// 1. 读取 /proc 目录下的所有进程目录, 例如 /proc/1, /proc/2, ...
// 2. 读取 /proc/<pid>/status 文件, 例如 /proc/1/status, /proc/2/status, ...
// 3. 读取 /proc/<pid>/cmdline 文件, 例如 /proc/1/cmdline, /proc/2/cmdline, ...
// 4. 输出进程信息
// 5. 默认只输出进程的 pid, ppid, name, cmdline 信息, 参考 ps 命令的输出格式

use std::collections::hash_map::Entry::{Occupied, Vacant};
use std::collections::HashMap;
use std::fs;
use std::fs::File;
use std::io::prelude::*;
use std::path::Path;

struct ProcessRecord {
    pid: i32,
    ppid: i32,
    name: String,
    cmdline: String,
    tty: String,
    time: String,
    user: String,
    cpu: i32,
    memory: i32,
    status: String,
}

fn get_process_records() -> Vec<ProcessRecord> {
    let mut records: Vec<ProcessRecord> = Vec::new();
    let mut pid_to_record = HashMap::new();
    let proc_dir = Path::new("/proc");
    for entry in fs::read_dir(proc_dir).unwrap() {
        let entry = entry.unwrap();
        let path = entry.path();
        if path.is_dir() {
            let pid = path
                .file_name()
                .unwrap()
                .to_str()
                .unwrap()
                .parse::<i32>()
                .unwrap();
            let status_path = path.join("status");
            let cmdline_path = path.join("cmdline");
            let status = fs::read_to_string(status_path).unwrap();
            let cmdline = fs::read_to_string(cmdline_path).unwrap();
            let mut record = ProcessRecord {
                pid: pid,
                ppid: 0,
                name: String::new(),
                cmdline: String::new(),
                tty: String::new(),
                time: String::new(),
                user: String::new(),
                cpu: 0,
                memory: 0,
                status: status,
            };
            for line in status.lines() {
                let parts: Vec<&str> = line.split_whitespace().collect();
                if parts.len() < 2 {
                    continue;
                }
                match parts[0] {
                    "PPid:" => {
                        record.ppid = parts[1].parse::<i32>().unwrap();
                    }
                    "Name:" => {
                        record.name = parts[1].to_string();
                    }
                    "Tty:" => {
                        record.tty = parts[1].to_string();
                    }
                    "Time:" => {
                        record.time = parts[1].to_string();
                    }
                    "Uid:" => {
                        record.user = parts[1].to_string();
                    }
                    _ => {}
                }
            }
            record.cmdline = cmdline;
            match pid_to_record.entry(pid) {
                Vacant(entry) => {
                    entry.insert(record);
                }
                Occupied(mut entry) => {
                    let mut record = entry.get_mut();
                    record.cmdline = cmdline;
                }
            }
        }
    }
    for (pid, record) in pid_to_record.iter() {
        // TODO
        records.push(record.clone());
    }
    records
}

fn main() {}
