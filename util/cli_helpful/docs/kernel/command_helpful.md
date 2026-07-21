# Linux Kernel Module Development Template

A clean template and essential command reference for developing, building, and debugging Linux Kernel Modules (LKMs).

## Prerequisites

Before building, install the required build tools and kernel headers for your system:

```bash
# Ubuntu / Debian
sudo apt update
sudo apt install build-essential linux-headers-\$(uname -r)

# Fedora / RHEL
sudo dnf groupinstall "Development Tools"
sudo dnf install kernel-devel-\$(uname -r)
```

## Quick Start Commands

### 1. Build & Installation

* **Compile the module:**
  ```bash
  make -C /lib/modules/(uname -r)/build M=(pwd) modules
  ```
* **Install into system directory:**
  ```bash
  sudo make modules_install
  ```
* **Update dependency list:**
  ```bash
  sudo depmod -a
  ```

### 2. Lifecycle Management

* **Insert module directly:**
  ```bash
  sudo insmod <file>.ko
  ```
* **Remove module safely:**
  ```bash
  sudo rmmod <name>
  ```
* **Load with dependencies:**
  ```bash
  sudo modprobe <name>
  ```
* **Remove with dependencies:**
  ```bash
  sudo modprobe -r <name>
  ```

### 3. Inspection & Debugging

* **List loaded modules:**
  ```bash
  lsmod
  ```
* **View module details:**
  ```bash
  modinfo <file>.ko
  ```
* **Monitor kernel logs (printk):**
  ```bash
  sudo dmesg -w
  ```
* **View systemd kernel journal:**
  ```bash
  journalctl -k -f
  ```

## Advanced Kernel Debugging & System Inspection

### 1. Storage & Block Device Verification
Before testing block drivers or partition manipulators, always verify the layout to avoid data corruption.
* **`lsblk -f`**: Checks the file system type, UUID, and mount points of all block devices. Use this to locate your target non-root partition.
* **`dd if=/dev/zero of=/dev/sdX bs=4M count=10 status=progress`**: **WARNING: Destructive.** Wipes partition tables or tests low-level write performance. Double-check the target device identifier before execution.

### 2. Device Nodes & Standard I/O Descriptors
Character and block drivers communicate via `/dev`. Review permissions, major/minor numbers, and symbolic links using:
* **`ls -la /dev/`**: Inspects device nodes. For example, standard descriptors link directly back to the process-specific file descriptor subdirectories in `/proc`:
  ```text
  lrwxrwxrwx   1 root root            15 Jul 22  2026 stderr -> /proc/self/fd/2
  lrwxrwxrwx   1 root root            15 Jul 22  2026 stdin -> /proc/self/fd/0
  lrwxrwxrwx   1 root root            15 Jul 22  2026 stdout -> /proc/self/fd/1
  ```
* **`cat /proc/devices`**: Lists all allocated character and block devices along with their assigned major numbers to prevent allocation conflicts.

### 3. Tracing, Memory, and Symbols
* **`cat /proc/kallsyms | grep <symbol_name>`**: Locates the exact dynamic memory address of kernel functions and variables.
* **`sudo cat /sys/kernel/debug/kmemleak`**: Scans for unreferenced kernel memory allocations to catch memory leaks early.
* **`strace -e trace=openat,ioctl <user_app>`**: Traces system calls made by your test application when interacting with your custom `/dev` node.


## License
GPL-2.0