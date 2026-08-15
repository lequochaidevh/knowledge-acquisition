# Compile
```sh
sudo apt update && sudo apt install device-tree-compiler
dtc -I dts -O dtb -o my_device_tree.dtb my_device_tree.dts
```
-I dts: Input format ís DTS.
-O dtb: Output format is DTB.
-o: output file name.

# Decompile
```sh
dtc -I dtb -O dts -o recovered.dts my_device_tree.dtb

# Get run time decompile
dtc -I fs -O dts -o live_system.dts /proc/device-tree
```
```sh
# check file device-tree dir
ls -l /sys/firmware/acpi/tables/
# or
ls -l /sys/devices/LNXSYSTM:00/

# 1. Intel
sudo apt install acpica-tools

# 2. Copy file
cp /sys/firmware/acpi/tables/DSDT ~/dsdt.dat

# 3. (Decompile) file source to .dsl
iasl -d ~/dsdt.dat
```

-I fs: Input format is File System

File .dtsi (Device Tree Source Include) 

# Device tree overlay
```cpp
/dts-v1/;
/plugin/; /* Obligated: to declare for compiler that is DeviceTree Overlay */

&i2c1 {
    status = "okay"; /* Active the controller I2C number 1 */

    #address-cells = <1>;
    #size-cells = <0>;

    /* Add new node device to bus I2C */
    sensor@48 {
        compatible = "ti,tmp102";
        reg = <0x48>; /* Address I2C sensor */
        status = "okay";
    };
};
```

```sh
dtc -@ -I dts -O dtb -o my_overlay.dtbo my_overlay.dts
```

```sh
# Load
# method 1:
sudo mkdir /sys/kernel/config/device-tree/overlays/my_node
sudo cat my_overlay.dtbo > /sys/kernel/config/device-tree/overlays/my_node/dtbo

# method 2:
# Copy file .dtbo to /boot/overlays/ (depend the platform)
dtoverlay=my_overlay
```

### Merge dtb and dtbo to dtb only.
```sh
fdtoverlay -i base.dtb -o merged_output.dtb overlay.dtbo
```