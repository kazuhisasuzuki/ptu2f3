# ptu2f3

This program controlls USB AC tap PTU3F3 (http://www.kairen.co.jp/japanese/other/index.html)
working on linux.

Base program comes from below site.

http://kentai-shiroma.blogspot.jp/2012/06/usbptu2f3linux.html

## Require
*libusb-devel

## Compile

```
# gcc -o ptu2f3 main.c -lusb
```

## Usage

### Status
below shows 2 devices connected case.

```
# ./ptu2f3 -s
2 devices found
Device 0 => Port 4:OFF, Port 5:OFF
Device 1 => Port 4:OFF, Port 5:OFF
```
*Device number is detemined by program, so please check detemined device number and actual device before use.

### Control

Port 4 ON for only one device connected

```
# ./ptu2f3 4 ON
```

Port 4 ON for device 1 when more than one device connected

```
# ./ptu2f3 -m 1 4 ON
```
