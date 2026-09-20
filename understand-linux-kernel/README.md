# Linux Kernel Learning Plan for DevOps Engineers

> Made by AI

## Overview
This plan is tailored for DevOps engineers with 6+ years of experience, familiar with Linux, NixOS, Python, and Go. It focuses on practical kernel understanding that directly applies to containerization, orchestration, and system performance.

## Prerequisites Met ✓
- [x] Linux system administration experience
- [x] NixOS familiarity (reproducible builds)
- [x] Python/Go programming skills
- [x] TLPI (The Linux Programming Interface) completed
- [x] Basic C/Rust knowledge

## Phase 1: Environment Setup & Boot Process Deep Dive (Week 1)

### Day 1-4: Boot Process Analysis

#### 1. BIOS/UEFI to Kernel Handoff
```bash
# Trace boot process
sudo dmesg | head -50
cat /proc/cmdline  # Kernel parameters from bootloader
ls -la /sys/firmware/efi/ # UEFI info
```

#### 2. Kernel Entry Points to Study
- **init/main.c:687** - `start_kernel()` - First C function
- **init/main.c:711** - `rest_init()` - Creates init process
- **init/main.c:900** - `kernel_init()` - Final init before userspace
- **init/initramfs.c:623** - `populate_rootfs()` - Initramfs handling

#### 3. Boot Process Timeline
```
BIOS/UEFI → Bootloader (GRUB) → Kernel Decompression → start_kernel() → 
init process (PID 1) → systemd → user space services
```

#### 4. Practical Exercise
```bash
# Create boot analysis script
#!/bin/bash
echo "=== Boot Process Analysis ==="
echo "1. Kernel version: $(uname -r)"
echo "2. Boot time: $(systemd-analyze time)"
echo "3. Critical chain:"
systemd-analyze critical-chain

echo "4. Kernel boot parameters:"
cat /proc/cmdline | tr ' ' '\n' | sort

echo "5. Early boot messages:"
sudo journalctl -b -k --priority=3
```

### Day 5-7: Documentation Deep Dive

#### Essential Documentation Files
1. **Documentation/admin-guide/README.rst** - Admin guide overview
2. **Documentation/core-api/boot-time-mm.rst** - Boot memory management
3. **Documentation/filesystems/ramfs-rootfs-initramfs.rst** - Initramfs details
4. **Documentation/admin-guide/kernel-parameters.txt** - All boot parameters

#### Reading Strategy
- **Morning (30 min)**: Read 1 documentation file
- **Evening (45 min)**: Trace code mentioned in docs
- **Weekend**: Apply findings to actual system

## Phase 2: Container Runtime Internals (Week 2)

### Day 1-2: Namespace Deep Dive

#### Key Files to Study
- **kernel/nsproxy.c:42** - Namespace proxy structures
- **kernel/pid_namespace.c:123** - PID namespace implementation
- **kernel/user_namespace.c:89** - User namespace security
- **net/core/net_namespace.c:67** - Network namespace

#### Practical Analysis
```go
// tools/namespace-tracer.go
package main

import (
    "fmt"
    "os"
    "os/exec"
    "strconv"
    "strings"
)

func main() {
    cmd := exec.Command("unshare", "--pid", "--fork", "--mount-proc", "ls", "/proc")
    cmd.Stdout = os.Stdout
    cmd.Stderr = os.Stderr
    
    fmt.Println("=== PID Namespace Demo ===")
    fmt.Printf("Parent PID: %d\n", os.Getpid())
    
    if err := cmd.Run(); err != nil {
        fmt.Printf("Error: %v\n", err)
    }
}
```

### Day 3-4: Cgroups v2 Implementation

#### Core Files
- **kernel/cgroup/cgroup.c:145** - Main cgroup framework
- **kernel/cgroup/cpuset.c:234** - CPU set management
- **kernel/cgroup/memory.c:567** - Memory controller
- **kernel/cgroup/pids.c:89** - PID controller

#### NixOS Integration
```nix
# Check current cgroup setup
{ pkgs, ... }:
{
  systemd.services.cgroup-analyzer = {
    serviceConfig.ExecStart = ''
      ${pkgs.bash}/bin/bash -c "
        echo '=== Cgroup Analysis ==='
        mount | grep cgroup
        cat /proc/cgroups
        find /sys/fs/cgroup -type d -name '*docker*' | head -5
      "
    '';
  };
}
```

### Day 5-7: Container Syscall Analysis

#### System Call Focus
- **clone()** - Process creation with flags
- **unshare()** - Create new namespaces
- **setns()** - Join existing namespaces
- **mount()** - Filesystem isolation
- **pivot_root()** - Change root filesystem

#### Tracing Script
```bash
#!/bin/bash
# trace-container-start.sh

CONTAINER_NAME="test-container"
TRACE_FILE="/tmp/container-trace.log"

echo "=== Tracing container startup ==="

# Start tracing
sudo strace -f -e trace=clone,unshare,setns,mount,pivot_root \
  -o "$TRACE_FILE" \
  docker run --rm --name "$CONTAINER_NAME" alpine sleep 5 &

# Wait for container
sleep 3

# Analyze trace
echo "=== Syscall Analysis ==="
grep -E "(clone|unshare|setns|mount|pivot_root)" "$TRACE_FILE" | \
  awk '{print $1, $2, $NF}' | sort -u
```

## Phase 3: Memory Management for Containers (Week 3)

### Day 1-2: Memory Cgroups

#### Key Implementation Files
- **mm/memcontrol.c:234** - Memory controller
- **mm/page_counter.c:89** - Page accounting
- **mm/vmpressure.c:145** - Memory pressure handling
- **mm/oom_kill.c:567** - OOM killer integration

#### Memory Analysis Tools
```python
#!/usr/bin/env python3
# memory_analyzer.py

import os
import json

def get_cgroup_memory_info():
    """Extract memory cgroup information"""
    memory_path = "/sys/fs/cgroup/memory"
    
    result = {}
    for container in os.listdir(memory_path):
        if container.startswith("docker"):
            container_path = os.path.join(memory_path, container)
            
            # Read memory stats
            stats_file = os.path.join(container_path, "memory.stat")
            if os.path.exists(stats_file):
                with open(stats_file) as f:
                    stats = {}
                    for line in f:
                        key, value = line.strip().split()
                        stats[key] = int(value)
                    
                    result[container] = {
                        "cache": stats.get("cache", 0),
                        "rss": stats.get("rss", 0),
                        "swap": stats.get("swap", 0),
                        "usage_in_bytes": int(open(os.path.join(container_path, "memory.usage_in_bytes")).read()),
                        "limit_in_bytes": int(open(os.path.join(container_path, "memory.limit_in_bytes")).read())
                    }
    
    return result

if __name__ == "__main__":
    print(json.dumps(get_cgroup_memory_info(), indent=2))
```

### Day 3-4: Virtual Memory in Containers

#### VM Subsystem Files
- **mm/mmap.c:1234** - Memory mapping
- **mm/mprotect.c:567** - Memory protection
- **mm/mlock.c:234** - Memory locking
- **mm/huge_memory.c:890** - Huge pages

#### Huge Pages for Containers
```bash
#!/bin/bash
# hugepages-setup.sh

echo "=== Huge Pages Analysis ==="
echo "Transparent Huge Pages: $(cat /sys/kernel/mm/transparent_hugepage/enabled)"
echo "Huge Pages Total: $(cat /proc/sys/vm/nr_hugepages)"
echo "Huge Pages Free: $(cat /proc/sys/vm/nr_hugepages)"

# Create container with huge pages
docker run --rm -it \
  --device=/dev/hugepages:/dev/hugepages \
  --shm-size=1g \
  alpine sh -c "
    echo '=== Inside container ==='
    mount | grep hugetlb
    cat /proc/meminfo | grep Huge
  "
```

### Day 5-7: OOM Handling

#### OOM Implementation Study
- **mm/oom_kill.c:123** - OOM killer selection logic
- **mm/memcontrol.c:890** - Cgroup OOM handling
- **kernel/cgroup/memory.c:456** - Memory event notifications

## Phase 4: Networking & eBPF (Week 4)

### Day 1-2: Container Networking

#### Network Namespace Files
- **net/core/net_namespace.c:234** - Network namespace implementation
- **net/bridge/br_netfilter.c:567** - Bridge netfilter
- **net/core/filter.c:123** - Socket filtering

#### Network Analysis
```python
#!/usr/bin/env python3
# network_namespace_analyzer.py

import os
import subprocess
import json

def get_network_namespaces():
    """List all network namespaces"""
    namespaces = []
    
    # Docker containers
    docker_containers = subprocess.check_output(
        ["docker", "ps", "-q"], text=True
    ).strip().split('\n')
    
    for container in docker_containers:
        if container:
            ns = subprocess.check_output(
                ["docker", "inspect", "--format", "{{.NetworkSettings.SandboxKey}}", container],
                text=True
            ).strip()
            
            # Get interface info
            interfaces = subprocess.check_output(
                ["docker", "exec", container, "ip", "link", "show"],
                text=True
            )
            
            namespaces.append({
                "container": container[:12],
                "namespace": ns,
                "interfaces": interfaces
            })
    
    return namespaces

if __name__ == "__main__":
    print(json.dumps(get_network_namespaces(), indent=2))
```

### Day 3-4: eBPF for Container Monitoring

#### eBPF Program Development
```c
// tools/container_monitor.bpf.c
#include <linux/bpf.h>
#include <linux/ptrace.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);  // PID
    __type(value, u64); // Syscall count
} syscall_count SEC(".maps");

SEC("tracepoint/raw_syscalls/sys_enter")
int trace_sys_enter(struct trace_event_raw_sys_enter *ctx)
{
    u32 pid = bpf_get_current_pid_tgid() >> 32;
    u64 *count, zero = 0;
    
    count = bpf_map_lookup_elem(&syscall_count, &pid);
    if (!count) {
        count = &zero;
    }
    
    (*count)++;
    bpf_map_update_elem(&syscall_count, &pid, count, BPF_ANY);
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

#### Building eBPF Programs
```bash
#!/bin/bash
# build-ebpf.sh

clang -O2 -target bpf -c container_monitor.bpf.c -o container_monitor.bpf.o
bpftool gen skeleton container_monitor.bpf.o > container_monitor.skel.h

# Go integration
cat > container_monitor.go << 'EOF'
package main

/*
#include "container_monitor.skel.h"
*/
import "C"

import (
    "fmt"
    "time"
)

func main() {
    obj := C.container_monitor_bpf__open_and_load()
    if obj == nil {
        panic("Failed to load BPF program")
    }
    defer C.container_monitor_bpf__destroy(obj)
    
    fmt.Println("eBPF container monitoring started...")
    
    for {
        time.Sleep(5 * time.Second)
        // Read map data and process
    }
}
EOF
```

### Day 5-7: Performance Analysis

#### Performance Monitoring
```bash
#!/bin/bash
# container-perf-analysis.sh

CONTAINER_ID="test-container"

echo "=== Container Performance Analysis ==="

# Start container
docker run -d --name "$CONTAINER_ID" alpine sh -c "while true; do echo hello; sleep 1; done"

# Monitor with perf
echo "1. Perf events:"
sudo perf stat -e syscalls:sys_enter_openat -p $(docker inspect --format '{{.State.Pid}}' "$CONTAINER_ID") sleep 5

# Monitor with BPF
echo "2. BPF trace:"
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_openat /comm == "sh"/ { @[pid] = count(); }' -c "docker exec $CONTAINER_ID ls"

# Cleanup
docker rm -f "$CONTAINER_ID"
```

## Phase 5: Advanced Topics (Week 5+)

### Security Modules
- **security/apparmor/** - AppArmor integration
- **security/selinux/** - SELinux implementation
- **security/seccomp/** - Seccomp filtering

### Filesystem Integration
- **fs/overlayfs/** - Overlay filesystem for containers
- **fs/btrfs/** - Btrfs subvolumes
- **fs/ext4/** - Ext4 for container storage

### Kernel Modules for Containers
```c
// custom_container_module.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/sched/task.h>

static ssize_t container_stats_read(struct file *file, char __user *buf, 
                                   size_t count, loff_t *ppos)
{
    char tmp[256];
    int len;
    
    len = snprintf(tmp, sizeof(tmp), 
                  "Running containers: %d\n", 
                  get_nr_containers());
    
    return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static const struct proc_ops container_stats_ops = {
    .proc_read = container_stats_read,
};

static int __init container_module_init(void)
{
    proc_create("container_stats", 0444, NULL, &container_stats_ops);
    return 0;
}

module_init(container_module_init);
MODULE_LICENSE("GPL");
```

## Daily Learning Routine

### Morning (30 min): Documentation Reading
1. **Monday**: Documentation/admin-guide/
2. **Tuesday**: Documentation/core-api/
3. **Wednesday**: Documentation/networking/
4. **Thursday**: Documentation/filesystems/
5. **Friday**: Documentation/cgroup-v2.rst

### Evening (45 min): Code Analysis
1. **Trace 1 kernel function** with GDB
2. **Read 100 lines** of relevant source
3. **Write 1 small test** program
4. **Document findings** in notes

### Weekend (2-3 hours): Hands-on Projects
1. **Build custom kernel** with NixOS
2. **Create eBPF programs** for monitoring
3. **Develop kernel modules** for container features
4. **Performance testing** with different configs

## Progress Tracking

### Week 1 Checklist
- [ ] NixOS kernel dev environment setup
- [ ] Boot process traced and documented
- [ ] Current kernel config analyzed
- [ ] First custom kernel built

### Week 2 Checklist
- [ ] Namespace implementation understood
- [ ] Cgroup v2 features mapped
- [ ] Container syscalls traced
- [ ] Go monitoring tool created

### Week 3 Checklist
- [ ] Memory management deep dive
- [ ] OOM handling understood
- [ ] Huge pages configured
- [ ] Memory analysis tools built

### Week 4 Checklist
- [ ] Network namespaces mapped
- [ ] eBPF programs deployed
- [ ] Performance analysis complete
- [ ] Security modules studied

## Resources & References

### Documentation
- **Documentation/admin-guide/** - System administration
- **Documentation/core-api/** - Core kernel APIs
- **Documentation/cgroup-v2.rst** - Cgroup documentation
- **Documentation/bpf/** - eBPF documentation

### Tools
- **cscope** - Code navigation
- **gdb** - Kernel debugging
- **perf** - Performance analysis
- **bpftrace** - eBPF tracing
- **qemu** - Kernel testing

### Community
- **Kernel Newbies mailing list**
- **LWN.net** - Kernel articles
- **NixOS kernel discussions**
- **Container runtime developer lists**

## Success Metrics

### Knowledge
- [ ] Can explain boot process from BIOS to PID 1
- [ ] Understands container isolation mechanisms
- [ ] Can debug memory cgroup issues
- [ ] Can write eBPF programs for monitoring

### Practical
- [ ] Custom kernel builds successfully
- [ ] Container performance improved
- [ ] Monitoring tools deployed
- [ ] Security policies implemented

### Contribution
- [ ] NixOS kernel module created
- [ ] Bug reports submitted
- [ ] Documentation improvements
- [ ] Tools shared with community
