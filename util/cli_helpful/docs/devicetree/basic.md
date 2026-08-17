
### Decompile
dtc -I dtb -O dts -o output_overlay.dts input_overlay.dtbo

### Merge overlay
fdoverlay -i base_tree.dtb -o merged_output.dtb overlay1.dtbo overlay2.dtbo
