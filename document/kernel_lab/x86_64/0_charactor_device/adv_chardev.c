#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/ioctl.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AI Lab");
MODULE_DESCRIPTION("Advanced Ring Buffer Char Driver for x86_64");
MODULE_VERSION("1.1");

#define DEVICE_NAME "adv_buffer"
#define MAX_STORAGE 512 /* Fixed storage size for seek logic */
#define BUFFER_SIZE 256 /* Dynamic buffer max size */

/* IOCTL Command Definitions using explicit macro layout */
#define MAGIC_NUM          'k'
#define IOCTL_RESET_BUFFER _IO(MAGIC_NUM, 1)
#define IOCTL_GET_STATUS   _IOR(MAGIC_NUM, 2, int)

/* Device state structure containing context and synchronization primitives */
struct advanced_dev {
    char *       buffer;
    size_t       head;
    size_t       tail;
    size_t       size;
    struct mutex buf_mutex;
    struct cdev  cdev;
};

static dev_t                dev_num;
static struct class *       dev_class = NULL;
static struct advanced_dev *my_dev    = NULL;

/* Open implementation */
static int dev_open(struct inode *inode, struct file *file) {
    /* Store device pointer inside private_data for subsequent read/write/ioctl operations */
    file->private_data = container_of(inode->i_cdev, struct advanced_dev, cdev);
    pr_info("%s: Device opened successfully\n", DEVICE_NAME);
    return 0;
}

/* Close implementation */
static int dev_release(struct inode *inode, struct file *file) {
    pr_info("%s: Device closed\n", DEVICE_NAME);
    return 0;
}

/* Read implementation (FIFO Ring Buffer) */
static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *off) {
    struct advanced_dev *dev = file->private_data;
    size_t               available, bytes_to_read;

    /* Acquire mutex to prevent race conditions during read pointer manipulation */
    if (mutex_lock_interruptible(&dev->buf_mutex)) return -ERESTARTSYS;

    available = dev->size;
    if (available == 0) {
        mutex_unlock(&dev->buf_mutex);
        return 0; /* EOF - No data to read */
    }

    bytes_to_read = (len > available) ? available : len;

    /* Handle ring buffer wrap-around scenario */
    if (dev->tail + bytes_to_read > BUFFER_SIZE) {
        size_t first_part  = BUFFER_SIZE - dev->tail;
        size_t second_part = -first_part;

        if (copy_to_user(buf, dev->buffer + dev->tail, first_part) ||
            copy_to_user(buf + first_part, dev->buffer, second_part)) {
            mutex_unlock(&dev->buf_mutex);
            return -EFAULT;
        }
    } else {
        if (copy_to_user(buf, dev->buffer + dev->tail, bytes_to_read)) {
            mutex_unlock(&dev->buf_mutex);
            return -EFAULT;
        }
    }

    /* Update ring buffer state */
    dev->tail = (dev->tail + bytes_to_read) % BUFFER_SIZE;
    dev->size -= bytes_to_read;

    mutex_unlock(&dev->buf_mutex);
    return bytes_to_read;
}

/* Write implementation (Overwrites old data if buffer is full) */
static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *off) {
    struct advanced_dev *dev            = file->private_data;
    size_t               bytes_to_write = (len > BUFFER_SIZE) ? BUFFER_SIZE : len;

    if (mutex_lock_interruptible(&dev->buf_mutex)) return -ERESTARTSYS;

    /* Wrap-around memory write logic */
    if (dev->head + bytes_to_write > BUFFER_SIZE) {
        size_t first_part  = BUFFER_SIZE - dev->head;
        size_t second_part = bytes_to_write - first_part;

        if (copy_from_user(dev->buffer + dev->head, buf, first_part) ||
            copy_from_user(dev->buffer, buf + first_part, second_part)) {
            mutex_unlock(&dev->buf_mutex);
            return -EFAULT;
        }
    } else {
        if (copy_from_user(dev->buffer + dev->head, buf, bytes_to_write)) {
            mutex_unlock(&dev->buf_mutex);
            return -EFAULT;
        }
    }

    dev->head = (dev->head + bytes_to_write) % BUFFER_SIZE;
    dev->size += bytes_to_write;
    if (dev->size > BUFFER_SIZE) {
        dev->size = BUFFER_SIZE;
        dev->tail = dev->head; /* Advance tail if buffer overflowed */
    }

    mutex_unlock(&dev->buf_mutex);
    return bytes_to_write;
}

/* Modern Unlocked IOCTL implementation for configuration control */
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct advanced_dev *dev = file->private_data;
    int                  current_occupancy;

    switch (cmd) {
        case IOCTL_RESET_BUFFER:
            if (mutex_lock_interruptible(&dev->buf_mutex)) return -ERESTARTSYS;
            dev->head = 0;
            dev->tail = 0;
            dev->size = 0;
            memset(dev->buffer, 0, BUFFER_SIZE);
            mutex_unlock(&dev->buf_mutex);
            pr_info("%s: IOCTL Reset triggered\n", DEVICE_NAME);
            break;

        case IOCTL_GET_STATUS:
            if (mutex_lock_interruptible(&dev->buf_mutex)) return -ERESTARTSYS;
            current_occupancy = (int)dev->size;
            mutex_unlock(&dev->buf_mutex);

            if (copy_to_user((int __user *)arg, &current_occupancy, sizeof(current_occupancy))) return -EFAULT;
            break;

        default:
            return -ENOTTY; /* Return appropriate invalid command error */
    }
    return 0;
}

static loff_t device_set_llseek(struct file *file, loff_t offset, int whence) {
    struct advanced_dev *dev = file->private_data;

    if (mutex_lock_interruptible(&dev->buf_mutex)) return -ERESTARTSYS;

    /* Ring buffers are stream devices. We only support seeking to the very beginning (0) */
    if (whence == SEEK_SET && offset == 0) {
        file->f_pos = 0;
        dev->head   = 0;
        dev->tail   = 0;
        dev->size   = 0;
        memset(dev->buffer, 0, BUFFER_SIZE);
        mutex_unlock(&dev->buf_mutex);
        pr_info("%s: Buffer reset via llseek (SEEK_SET, 0)\n", DEVICE_NAME);
        return 0;
    }

    mutex_unlock(&dev->buf_mutex);
    return -ESPIPE; /* Illegal seek error code standard for pipes/streams */
}

/* File operations mapping bind matrix */
static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = dev_open,
    .release        = dev_release,
    .read           = dev_read,
    .write          = dev_write,
    .unlocked_ioctl = dev_ioctl,
    .llseek         = device_set_llseek,
};

static int __init adv_init(void) {
    /* Allocate major/minor numbers dynamically */
    if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0) return -1;

    /* Create dynamic sysfs device class descriptor */
    dev_class = class_create(THIS_MODULE, "adv_char_class");
    if (IS_ERR(dev_class)) {
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(dev_class);
    }

    /* Allocate custom device instance context from heap */
    my_dev = kzalloc(sizeof(struct advanced_dev), GFP_KERNEL);
    if (!my_dev) {
        class_destroy(dev_class);
        unregister_chrdev_region(dev_num, 1);
        return -ENOMEM;
    }

    my_dev->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);
    mutex_init(&my_dev->buf_mutex);

    /* Initialize character device interface link mapping matrix */
    cdev_init(&my_dev->cdev, &fops);
    if (cdev_add(&my_dev->cdev, dev_num, 1) < 0) {
        kfree(my_dev->buffer);
        kfree(my_dev);
        class_destroy(dev_class);
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }

    /* Instantiate localized target device entry point in /dev directory */
    if (IS_ERR(device_create(dev_class, NULL, dev_num, NULL, DEVICE_NAME))) {
        cdev_del(&my_dev->cdev);
        kfree(my_dev->buffer);
        kfree(my_dev);
        class_destroy(dev_class);
        unregister_chrdev_region(dev_num, 1);
        return -1;
    }

    pr_info("%s: Loaded. Major=%d, Minor=%d\n", DEVICE_NAME, MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

static void __exit adv_exit(void) {
    device_destroy(dev_class, dev_num);
    cdev_del(&my_dev->cdev);
    mutex_destroy(&my_dev->buf_mutex);
    kfree(my_dev->buffer);
    kfree(my_dev);
    class_destroy(dev_class);
    unregister_chrdev_region(dev_num, 1);
    pr_info("%s: Unloaded safely\n", DEVICE_NAME);
}

module_init(adv_init);
module_exit(adv_exit);