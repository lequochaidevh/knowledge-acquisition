#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HarisDocs");
MODULE_DESCRIPTION("Hardware Simulation Driver with Multi-Attribute Sysfs");
MODULE_VERSION("2.0");

/* Hardware Limits */
#define MIN_FREQ_MHZ 100
#define MAX_FREQ_MHZ 4000
#define MAX_MODE_LEN 16

/* Global Hardware States */
static unsigned int m_frequency_mhz            = 1200;
static unsigned int m_power_mwatts             = 2500;
static int          m_temperature_c            = 45;
static bool         m_is_enabled               = true;
static char         m_power_mode[MAX_MODE_LEN] = "balanced";

/* Mutex to protect shared hardware states */
static DEFINE_MUTEX(hw_lock);

static struct kobject *m_hw_kobj;

/* 1. Frequency Attribute (Read-Write with Validation) */
static ssize_t freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    unsigned int val;
    mutex_lock(&hw_lock);
    val = m_frequency_mhz;
    mutex_unlock(&hw_lock);
    return sysfs_emit(buf, "%u MHz\n", val);
}

static ssize_t freq_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    unsigned int val;
    int          ret = kstrtouint(buf, 10, &val);
    if (ret < 0) return ret;

    /* Range validation */
    if (val < MIN_FREQ_MHZ || val > MAX_FREQ_MHZ) {
        pr_warn("hw_sim: Frequency %u out of range (%d-%d)\n", val, MIN_FREQ_MHZ, MAX_FREQ_MHZ);
        return -EINVAL;
    }

    mutex_lock(&hw_lock);
    m_frequency_mhz = val;
    mutex_unlock(&hw_lock);
    return count;
}

/* 2. Power Attribute (Read-Only) */
static ssize_t power_consumption_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    unsigned int val;
    mutex_lock(&hw_lock);
    val = m_power_mwatts;
    mutex_unlock(&hw_lock);
    return sysfs_emit(buf, "%u mW\n", val);
}

/* 3. Temperature Attribute (Read-Only) */
static ssize_t temperature_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    int val;
    mutex_lock(&hw_lock);
    val = m_temperature_c;
    mutex_unlock(&hw_lock);
    return sysfs_emit(buf, "%d C\n", val);
}

/* 4. Enable Status (Boolean Read-Write) */
static ssize_t enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    bool val;
    mutex_lock(&hw_lock);
    val = m_is_enabled;
    mutex_unlock(&hw_lock);
    return sysfs_emit(buf, "%d\n", val);
}

static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    bool val;
    int  ret = kstrtobool(buf, &val);
    if (ret < 0) return ret;

    mutex_lock(&hw_lock);
    m_is_enabled = val;
    /* Simulate hardware action based on status */
    if (!m_is_enabled) {
        m_frequency_mhz = 0;
        m_power_mwatts  = 0;
    } else {
        m_frequency_mhz = 1200;
        m_power_mwatts  = 2500;
    }
    mutex_unlock(&hw_lock);
    return count;
}

/* 5. Power Mode (String Read-Write) */
static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    ssize_t len;
    mutex_lock(&hw_lock);
    len = sysfs_emit(buf, "%s\n", m_power_mode);
    mutex_unlock(&hw_lock);
    return len;
}

static ssize_t mode_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    mutex_lock(&hw_lock);
    strscpy(m_power_mode, buf, MAX_MODE_LEN);
    strim(m_power_mode); /* Remove trailing whitespaces/newlines */

    /* Dynamically adjust hardware profiles based on mode */
    if (strcmp(m_power_mode, "performance") == 0) {
        m_frequency_mhz = 4000;
        m_power_mwatts  = 8000;
    } else if (strcmp(m_power_mode, "powersave") == 0) {
        m_frequency_mhz = 800;
        m_power_mwatts  = 1000;
    }
    mutex_unlock(&hw_lock);
    return count;
}

/* Macro definitions mapping node names to permissions and callbacks */
static struct kobj_attribute freq_attr  = __ATTR(frequency, 0660, freq_show, freq_store);
static struct kobj_attribute power_attr = __ATTR_RO(power_consumption);
static struct kobj_attribute temp_attr  = __ATTR_RO(temperature);
// OR:
/* Pass the file name, read-only permissions (0444), the custom show function, and NULL for store */
// static struct kobj_attribute power_attr  = __ATTR(power_consumption, 0444, power_show, NULL);
// static struct kobj_attribute temp_attr   = __ATTR(temperature, 0444, temp_show, NULL);
static struct kobj_attribute enable_attr = __ATTR(enable, 0660, enable_show, enable_store);
static struct kobj_attribute mode_attr   = __ATTR(power_mode, 0660, mode_show, mode_store);

/* Attribute Group Assembly */
static struct attribute *hw_attrs[] = {
    &freq_attr.attr, &power_attr.attr, &temp_attr.attr, &enable_attr.attr, &mode_attr.attr, NULL,
};

static const struct attribute_group hw_attr_group = {
    .attrs = hw_attrs,
};

/* Subsystem Init */
static int __init hw_sim_init(void) {
    int error;

    m_hw_kobj = kobject_create_and_add("hw_simulator", kernel_kobj);
    if (!m_hw_kobj) return -ENOMEM;

    error = sysfs_create_group(m_hw_kobj, &hw_attr_group);
    if (error) {
        kobject_put(m_hw_kobj);
        return error;
    }

    pr_info("hw_simulator: Control interface created at /sys/kernel/hw_simulator/\n");
    return 0;
}

/* Subsystem Exit */
static void __exit hw_sim_exit(void) {
    kobject_put(m_hw_kobj);
    pr_info("hw_simulator: Driver unloaded\n");
}

module_init(hw_sim_init);
module_exit(hw_sim_exit);
