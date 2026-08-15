
# Documentation for file system.

### File process

file descriptor table | open file table | inode table

file descriptor table
- fd flag.
- file ptr.

open file table
- file offset (seek_set)
- status flag
- inode ptr

inode table
- inode file number
- file type
- size
- hardlink
- permission
- file ptr

only once file descriptor -> openfile table = dup()
two file descriptor -> openfile table = fork()

### read/write
- Latency (delay)
-> IOBUFERING : 
    read            write
        \          /
        Page cache
            |
        Disk/Flash

sync(), fsync()

III Synchonization

- semaphore
    + system V semaphore
    + Posix semaphore
        - named semaphore
        - unamed semaphore
- file lock
    + f lock (flock()): 1 session.
    + record lock (fcntl()): multi session (lseek)
