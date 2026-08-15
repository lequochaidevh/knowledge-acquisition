#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MAGIC_NUM          'k'
#define IOCTL_RESET_BUFFER _IO(MAGIC_NUM, 1)
#define IOCTL_GET_STATUS   _IOR(MAGIC_NUM, 2, int)

int main() {
    int fd = open("/dev/adv_buffer", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    int bytes_in_buffer = 0;
    // Check resource (byte) used in Kernel
    ioctl(fd, IOCTL_GET_STATUS, &bytes_in_buffer);
    printf("[UserSpace] Current occupancy in kernel buffer: %d bytes\n", bytes_in_buffer);

    printf("[UserSpace] Triggering Buffer Reset via IOCTL...\n");
    ioctl(fd, IOCTL_RESET_BUFFER);

    // Check after release resource
    ioctl(fd, IOCTL_GET_STATUS, &bytes_in_buffer);
    printf("[UserSpace] Occupancy after reset: %d bytes\n", bytes_in_buffer);

    close(fd);
    return 0;
}
