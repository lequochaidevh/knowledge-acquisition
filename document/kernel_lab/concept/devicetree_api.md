## Device Tree Parsing & Node Resolution Reference

When developing platform or hardware-abstracted drivers, Linux utilizes Open Firmware (OF) APIs to traverse the Device Tree Structure (DTS). All lookup APIs return a pointer to `struct device_node *`.

### 1. Core Node Retrieval APIs

| Target Criteria | Kernel API | Memory Management Rule |
| :--- | :--- | :--- |
| **Compatible String** | `of_find_compatible_node(from, type, compatible)` | Increases `refcount`. Must call `of_node_put()`. |
| **Absolute Path** | `of_find_node_by_path(path)` | Increases `refcount`. Must call `of_node_put()`. |
| **Node Name** | `of_find_node_by_name(from, name)` | Increases `refcount`. Must call `of_node_put()`. |
| **Device Type** | `of_find_node_by_type(from, type)` | Increases `refcount`. Must call `of_node_put()`. |
| **Phandle Target** | `of_parse_phandle(np, phandle_name, index)` | Increases `refcount`. Must call `of_node_put()`. |
| **Child Iteration** | `of_get_next_child(node, prev_child)` | Automatically decrements `prev_child` and increments new child. |

> ⚠️ **CRITICAL MEMORY RULE:** Every call to `of_find_*` or `of_get_*` increments the internal node reference counter. To prevent kernel memory leaks, you **must** release the pointer using `of_node_put(node_ptr)` once your configuration is parsed.

### 2. Code Implementation Blueprint

```c
#include <linux/of.h>

struct device_node *np;

/* Lookup the hardware node via the hardware compatibility matrix string */
np = of_find_compatible_node(NULL, NULL, "vendor,custom-hardware-device");
if (!np) {
    pr_err("Target device tree node not found\n");
    return -ENODEV;
}

/* Operational Phase: Parse configurations from properties here... */
pr_info("Found node name: %s\n", np->name);

/* Mandatory Cleanup: Decrement target reference counters */
of_node_put(np);
```

```cpp
/ {
    model = "DeviceTree Lab Evaluation Board x86_64";
    compatible = "ailab,eval-board", "qemu,x86_64";
    #address-cells = <1>;
    #size-cells = <1>;

    /* Node */
    my_custom_device@40000000 {
        compatible = "ailab,custom-sensor";
        reg = <0x40000000 0x1000>;  /* Address and this size */
        status = "okay";
        device-name = "Turbo-Sensor-X"; /* Custom feld */
        
        /* (u32 array) */
        sensor-thresholds = <10 50 100>; 
        
        /* Charactor */
        vendor-signature = "DeviceTree-LAB-2026";
    };
};
```

```cpp

static const struct of_device_id my_of_match[] = {
    { .compatible = "ailab,custom-sensor", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_of_match);

struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
// res->start auto = 0x40000000
// resource_size(res) auto = 0x1000

const char *custom_name;
if (of_property_read_string(np, "device-name", &custom_name) == 0) {
    pr_info("Custom Device Name is: %s\n", custom_name); // Turbo-Sensor-X
}

u32 thresholds[3];
if (of_property_read_u32_array(np, "sensor-thresholds", thresholds, 3) == 0) {
    pr_info("Threshold 2 is: %d\n", thresholds[1]); // 50
}

struct resource res;
if (of_address_to_resource(np, 0, &res) == 0) {
    pr_info("Base Address: 0x%pa, Size: %lld\n", &res.start, (long long)resource_size(&res));
    // Base Address: 0x40000000, Size: 4096
}

static int my_driver_probe(struct platform_device *pdev)
{
    struct resource *res;
    void __iomem *io_base;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "Could not get memory resource\n");
        return -EINVAL;
    }

    io_base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(io_base)) {
        return PTR_ERR(io_base);
    }

    dev_info(&pdev->dev, "Driver successfully matched and memory mapped!\n");
    return 0;
}
```


```cpp
/dts-v1/;

/ {
    model = "DeviceTree Lab Evaluation Board x86_64";
    compatible = "ailab,eval-board", "qemu,x86_64";
    #address-cells = <1>;
    #size-cells = <1>;

    /* 1. Define REF (Target Node) */
    /* (label) 'hardware_led' of node */
    hardware_led: led_controller@50000000 {
        compatible = "ailab,pwm-leds";
        reg = <0x50000000 0x100);
        status = "okay";
    };

    /* 2.  (Consumer Node) */
    my_custom_device@40000000 {
        compatible = "ailab,custom-sensor";
        reg = <0x40000000 0x1000>;
        status = "okay";
        
        /*  '&' + (label) to ref */
        led-control = <&hardware_led>; /* Note: ARRAY */
    };
};
```

```cpp
#include <linux/of.h>

static int parse_device_tree_reference(struct device_node *np) {
    struct device_node *led_np;

    /*
     * of_parse_phandle() takes three arguments:
     * - np: Pointer to the current consumer node (my_custom_device)
     * - "led-control": The property name storing the phandle in the .dts file
     * - 0: The index, in case the property holds an array of multiple references
     */
    led_np = of_parse_phandle(np, "led-control", 0);
    if (!led_np) {
        pr_err("Failed to find 'led-control' phandle reference\n");
        return -ENODEV;
    }

    /* Reference resolved successfully; printing target node name for verification */
    pr_info("Successfully resolved reference! Linked node name: %s\n", led_np->name); 
    /* Expected dmesg log: Linked node name: led_controller */

    /* 
     * MANDATORY MEMORY RULE: of_parse_phandle automatically increments the refcount 
     * of the target node (led_np). To avoid dynamic kernel memory leaks, you must 
     * release the pointer using of_node_put() once processing is finished.
     */
    of_node_put(led_np);

    return 0;
}
```

```cpp
    #address-cells = <1>; /* 32 bit */
    #size-cells = <1>; /* 32 bit */

    my_custom_device@40000000 {
        reg = <0x40000000 0x1000>; 
    };

    #address-cells = <2>; /* Need to 2 page 32-bit = 64-bit */
    #size-cells = <2>;    /* Need to 2 page 32-bit = 64-bit */

    my_custom_device@40000000 {
        /* reg = <HI_ADDR LOW_ADDR HI_SIZE LOW_SIZE> */
        reg = <0x00000000 0x40000000 0x00000000 0x00001000>;
    };
```
