# Simple character device driver
__

This is an example of a really basic character device driver for linux.
The device driver has an internal buffer to store messages, the buffer can be written to and read from.

## Usage
* To use this driver simply change to the directory of the module and type make.
* Once the module has been created load it into your kernel with insmod, insmod char_drv.ko
* The kernel will dynamically allocate a major number for this driver, now you have to create a device for the driver by typing 'mknod /dev/char_drv c MAJOR 0'
* Once you are done with the device remove both the device and the driver by typing 'rm -r /dev/char_drv' followed by 'rmmod char_drv'
