#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main() {
    // Open the character device
    int fd = open("/dev/adv_buffer", O_RDWR);
    if (fd < 0) {
        perror("[-] Failed to open /dev/adv_buffer");
        return -1;
    }
    printf("[+] Successfully opened device\n");

    // 1. Write some initial data into the ring buffer
    char *  data    = "Kernel_Stream_Data";
    ssize_t written = write(fd, data, strlen(data));
    printf("[User] Wrote %ld bytes: %s\n", (long)written, data);

    // 2. Try an INVALID seek (e.g., jump forward 10 bytes) -> Expect Failure
    printf("[User] Testing invalid seek (Forward 10 bytes)...\n");
    off_t invalid_offset = lseek(fd, 10, SEEK_CUR);
    if (invalid_offset == (off_t)-1) {
        // ESPIPE (Illegal seek) is the standard error for pipe/stream devices
        if (errno == ESPIPE) {
            printf("[+] Success: Kernel correctly rejected invalid seek with ESPIPE (Illegal seek).\n");
        } else {
            perror("[-] Unexpected error on invalid seek");
        }
    }

    // 3. Trigger VALID seek (Reset to 0) -> Expect Success
    printf("[User] Triggering buffer reset via lseek(fd, 0, SEEK_SET)...\n");
    off_t valid_offset = lseek(fd, 0, SEEK_SET);
    if (valid_offset == 0) {
        printf("[+] Success: Kernel cleared the ring buffer!\n");
    } else {
        perror("[-] Failed to execute valid lseek");
    }

    // 4. Verify by reading from the device -> Expect 0 bytes (EOF) because it was reset
    char    read_buf[32] = {0};
    ssize_t bytes_read   = read(fd, read_buf, sizeof(read_buf) - 1);
    printf("[User] Read attempt after reset: %ld bytes read. Content: '%s'\n", (long)bytes_read, read_buf);

    close(fd);
    return 0;
}
