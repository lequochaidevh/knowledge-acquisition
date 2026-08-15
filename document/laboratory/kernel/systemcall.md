## Linux System Call API Reference

This section provides a structured overview of the core Linux System Call APIs, detailing their official C function signatures from the `glibc` wrapper layer alongside explicit descriptions.

### 1. Process Control
* **`pid_t fork(void);`**
  Creates a child process by duplicating the calling process layout.
* **`int execve(const char *pathname, char *const _Nullable argv[], char *const _Nullable envp[]);`**
  Executes an entirely new binary file payload within the current process context.
* **`void exit(int status);`**
  Terminates the executing thread or calling process execution immediately.
* **`pid_t waitpid(pid_t pid, int *wstatus, int options);`**
  Forces a parent execution block until a child process changes state.
* **`pid_t getpid(void);`**
  Obtains the unique process identification number of the caller.

### 2. File Management
* **`int openat(int dirfd, const char *pathname, int flags, mode_t mode);`**
  Opens or creates an active target system file relative to a directory file descriptor.
* **`ssize_t read(int fd, void *buf, size_t count);`**
  Pulls a specified buffer byte chunk array out from a resource descriptor.
* **`ssize_t write(int fd, const void *buf, size_t count);`**
  Pushes an arbitrary length storage buffer target down into an active descriptor.
* **`int close(int fd);`**
  Releases tracking reference hooks for a specific numeric system resource identifier.
* **`off_t lseek(int fd, off_t offset, int whence);`**
  Repositions explicit cursor position metrics inside an open seekable data handle.

### 3. Device Management
* **`int ioctl(int fd, unsigned long request, ...);`**
  Executes hardware control commands bypassing regular filesystem abstractions.
* **`ssize_t readv(int fd, const struct iovec *iov, int iovcnt);`**
  Gathers incoming streamed driver data blocks directly into broken-up target locations.
* **`ssize_t writev(int fd, const struct iovec *iov, int iovcnt);`**
  Scatters disparate outbound data payloads straight into a single unified output stream.

### 4. Memory Management
* **`void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);`**
  Maps file storage blocks directly into active program memory bounds.
* **`int munmap(void *addr, size_t length);`**
  Tears down established runtime process address space memory mapping segments.
* **`int brk(void *addr);`**
  Resizes target data pointer boundary limits to grow or shrink heap allocations.

### 5. Information Maintenance
* **`int clock_gettime(clockid_t clk_id, struct timespec *tp);`**
  Exposes active timestamp parameters managed by high-resolution timer units.
* **`int uname(struct utsname *buf);`**
  Fills structures containing detailed operating kernel version metadata records.

### 6. Communication & Networking
* **`int socket(int domain, int type, int protocol);`**
  Spawns unmapped endpoint sockets for local or network communications.
* **`int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);`**
  Binds a transport pipe socket channel directly to a remote listening destination.
* **`int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);`**
  Locks an abstract communication socket down to a distinct hardware interface point.
* **`int listen(int sockfd, int backlog);`**
  Sets a bound socket node to begin welcoming sequential network connections.
* **`int accept(int sockfd, struct sockaddr *_Nullable restrict addr, socklen_t *_Nullable restrict addrlen);`**
  Pulls fresh active incoming peer sockets off the global incoming queue.
* **`int pipe(int pipefd[2]);`**
  Generates paired read/write pipeline descriptors for immediate Inter-Process Communication.

### 7. Security & Permissions
* **`int chmod(const char *pathname, mode_t mode);`**
  Re-evaluates and updates atomic user access permission bits on target files.
* **`int chown(const char *pathname, uid_t owner, gid_t group);`**
  Re-assigns fundamental ownership profile configurations governing file access.
