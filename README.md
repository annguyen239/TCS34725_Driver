
![Logo](https://upload.wikimedia.org/wikipedia/commons/b/b9/Logo_Tr%C6%B0%E1%BB%9Dng_%C4%90%E1%BA%A1i_H%E1%BB%8Dc_S%C6%B0_Ph%E1%BA%A1m_K%E1%BB%B9_Thu%E1%BA%ADt_TP_H%E1%BB%93_Ch%C3%AD_Minh.png)


## Authors

-  [Nguyễn Ngọc An - 22146259](https://github.com/annguyen239)
 - [Trần Hữu Khánh Toàn - 22146423](https://github.com/toanbestcl)
 - [Nguyễn Lê Minh Trí - 22146426](https://github.com/minhtri04)


# TCS34725_Driver
The TCS34725 driver for Raspberry Pi is a Python-based interface that allows the Pi to communicate with the TCS34725 color sensor over the I2C bus. It provides functions to read RGB and clear light values, configure integration time and gain, and control the onboard white LED for consistent illumination. This driver is commonly used in Pi-based projects involving color recognition, ambient light sensing, and basic object classification.


## Configuration (Raspberry Pi 5)

- Cd to firmware

```bash
  cd /boot/firmware
```
- Find file bcm2712-rpi-5-b.dtb
- Change bcm2712-rpi-5-b.dtb to .dts
```bash
  sudo dtc -I dtb -O dts -o bcm2712-rpi-5-b.dts bcm2712-rpi-5-b.dtb
```
- Access bcm2712-rpi-5-b.dts 
```bash
  sudo nano bcm2712-rpi-5-b.dts
```
- Find line: i2c@74000
- Add codes
```bash
  tcs34725@29 {
        compatible = "ti,tcs34725";
        reg = <0x29>;
        /* Optionally set integration time and gain
           atime = <0xd5>;     // ~101 ms
           gain = <0x01>;      // 4× gain
        */
    };
```
- Save, exit and change bcm2712-rpi-5-b.dts to .dtb
```bash
  sudo dtc -I dts -O dtb -o bcm2712-rpi-5-b.dtb bcm2712-rpi-5-b.dts
```

## Installation for driver
- Access folder
- Change obj-m to your filename:
```bash
  obj-m += tcs34725_driver.o
```
- Run Makefile:
```bash
  make
```
- After run "make", install module:
```bash
  sudo insmod tcs34725_driver.ko
```
- If you want to remove module, run:
```bash
  sudo rmmod tcs34725_driver
```

## Installation for ioctl driver
- Access folder
- Change obj-m to your filename:
```bash
  obj-m += tcs34725_ioctl_driver.o
```
- Run Makefile:
```bash
  make
```
- After run "make", install module:
```bash
  sudo insmod tcs34725_ioctl_driver.ko
```
- If you want to remove module, run:
```bash
  sudo rmmod tcs34725_ioctl_driver
```

## Run demo code
- Access folder
- Run Makefile:
```bash
  make run
```
- To stop running, use ctrl + C
