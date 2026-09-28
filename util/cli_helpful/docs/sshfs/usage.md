
## Usage

### 1. Mounting a Remote Directory
To mount a remote directory to your local file system, use the following syntax:

```bash
sshfs [username]@[hostname]:[remote_directory] [local_mount_point]
```

**Example:**
```bash
sshfs user@remotehost:/home/user /mnt/remote
```
*This command mounts the `/home/user` folder from the `remotehost` server directly into your local `/mnt/remote` directory.*

### 2. Unmounting the Directory
When you are done working, safely detach the remote file system using the `fusermount` utility:

```bash
fusermount -u /mnt/remote
```
*(For macOS users, use `umount /mnt/remote` instead).*

---

## Advanced Options

You can append flags to the `sshfs` command to fine-tune performance and access control.

### ⚡ Performance Optimization
SSHFS can sometimes experience lag over high-latency networks. Use these parameters to speed up data transfers and lower directory-loading delays:

```bash
sshfs user@remotehost:/remote/dir /local/dir \
  -o cache=yes \
  -o kernel_cache \
  -o compression=no \
  -o large_read
```
* **`-o cache=yes` & `-o kernel_cache`:** Caches file and directory metadata on your local system to prevent lag when re-opening folders.
* **`-o compression=no`:** Disables SSH compression. Recommended for fast networks (LAN/Fiber) to reduce local CPU overhead.
* **`-o large_read`:** Requests larger data blocks at once to maximize bandwidth utilization.

### 🔒 Access Control (Read/Write Modes)
By default, SSHFS mounts with read-write permissions. You can restrict this if needed:
* **Read-Only Mode:** Prevent accidental modifications to critical server files.
  ```bash
  sshfs user@remotehost:/remote/dir /local/dir -o ro
  ```

---