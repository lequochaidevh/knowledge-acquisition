make

sudo insmod adv_chardev.ko

ls -la /dev/adv_buffer

sudo chmod 666 /dev/adv_buffer

sudo dmesg -w

echo "Machine Spirit - Test x86_64 Kernel 5.15" > /dev/adv_buffer

cat /dev/adv_buffer

sudo rmmod adv_chardev

