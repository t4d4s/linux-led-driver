# 💡👻 Linux Led Driver

> A Linux kernel module that exposes an LED control interface via the `/dev` filesystem.

---

## Overview

`Linux Led Driver` is a loadable kernel module (LKM) written in C for Linux. It registers a character device that allows user-space programs to control an LED by reading and writing to a device file — a practical introduction to Linux kernel module development and the driver model.

## Repository Structure

```
linux-led-driver/
├── led_driver.c    # Kernel module source code
├── Makefile        # Kernel build configuration
```

---

## Requirements

- Linux kernel headers matching your running kernel
- GCC and GNU Make
- Root privileges (for loading/unloading the module)

On Ubuntu/Debian:

```bash
sudo apt install build-essential linux-headers-$(uname -r)
```

---

## Building

```bash
git clone https://github.com/t4d4s/linux-led-driver.git
cd linux-led-driver
make
```

This produces `led_driver.ko` — the compiled kernel module.

---

## Usage

### Load the module

```bash
sudo insmod led_driver.ko
```

### Confirm it loaded

```bash
dmesg | tail
lsmod | grep led_driver
```

### Interact with the device

Once loaded, the driver registers a character device under `/dev`. Write to it to control the LED:

```bash
echo "on"  | sudo tee /dev/led_driver
echo "off" | sudo tee /dev/led_driver
echo "interval 1" > /dev/led_driver
echo "1;5" > /dev/led_driver
cat /dev/led_driver
```

### Unload the module

```bash
sudo rmmod led_driver
```

---

## Cleaning Build Outputs

```bash
make clean
```

This removes all generated `.ko`, `.o`, `.mod`, and `.cmd` files.

---

## Contributions

Contributions are welcome! If you'd like to improve the project or add new features, please submit a pull request.

---

## Author

This project is maintained by [Tadas](https://github.com/t4d4s). Feel free to reach out with any questions or feedback.
