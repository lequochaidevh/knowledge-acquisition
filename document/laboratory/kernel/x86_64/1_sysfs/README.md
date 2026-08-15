```sh
# 1. Load the compiled kernel module
sudo insmod mymodule.ko

# 2. Navigate to the generated sysfs interface directory
cd /sys/kernel/hw_simulator/
ls -l
# Expected output: 5 files (enable, frequency, power_consumption, power_mode, temperature)

# 3. Read initial default hardware states
cat frequency          # Expected: 1200 MHz
cat power_consumption  # Expected: 2500 mW
cat temperature        # Expected: 45 C
cat power_mode         # Expected: balanced

# 4. Test Write Validation (Valid Input)
echo "2500" | sudo tee frequency
cat frequency          # Expected updated output: 2500 MHz

# 5. Test Write Validation (Out of Range Input)
echo "5000" | sudo tee frequency
# Expected system error: "tee: frequency: Invalid argument"
sudo dmesg | tail -n 2 # Check kernel warnings

# 6. Test Hardware Profile Logic Linkage via Power Mode
echo "performance" | sudo tee power_mode
cat frequency          # Expected auto-scaling: 4000 MHz
cat power_consumption  # Expected auto-scaling: 8000 mW

# 7. Test Read-Only File Restrictions
echo "60" | sudo tee temperature
# Expected system error: "tee: temperature: Permission denied"

# 8. Test Global Power Off State
echo "0" | sudo tee enable
cat frequency          # Expected drop: 0 MHz
cat power_consumption  # Expected drop: 0 mW

# 9. Clean up and unload the module from kernel memory
cd ~
sudo rmmod mymodule
```
