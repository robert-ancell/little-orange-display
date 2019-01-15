# Little Orange Display

This is a small project to show how to build a basic IoT device using
Ubuntu Core.

To build it you need the followig:
- A [Raspberry Pi](https://www.raspberrypi.org/products/raspberry-pi-3-model-a-plus/) (with power supply and SD card)
- A [Scroll pHat HD](https://shop.pimoroni.com/products/scroll-phat-hd)
- [Ubuntu Core 18](https://www.ubuntu.com/download/iot/raspberry-pi-2-3)

Install [this snap](https://snapcraft.io/little-orange-display) to see a continuously scrolling message on the display:
```
$ snap install little-orange-display
$ snap connect little-orange-display:i2c pi:i2c-1
$ snap start little-orange-display
```
