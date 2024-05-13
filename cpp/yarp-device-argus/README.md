
![YARP logo](https://raw.githubusercontent.com/robotology/yarp/master/doc/images/yarp-robot-24.png "yarp-device-argus")
yarp-device-argus
=================

This is the [argus](FIXME) device for [YARP](https://www.yarp.it/).
It supports the FIXME

The **FRAMOS™** cameras currently compatible with YARP are:
- FIXME

# 1. License
-------

[![License](https://img.shields.io/badge/license-BSD--3--Clause%20%2B%20others-19c2d8.svg)](https://github.com/robotology/yarp-device-realsense2/blob/master/LICENSE)

This software may be modified and distributed under the terms of the
BSD-3-Clause license. See the accompanying LICENSE file for details.

The argusCamera device uses the
[argus]FIXME sdk, released
under the [argus license](https://docs.baslerweb.com/licensing-information).
See the relative documentation for the terms of the license.

# 2. How to use argus cameras as a YARP device

## 2.1. Dependencies
Before proceeding further, please install the following dependencies:
- FIXME

## 2.2. Build and install yarp-device-argus

```bash
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=<installation_path> ..
make
make install
```
In order to make the device detectable, add `<installation_path>/share/yarp` to the `YARP_DATA_DIRS` environment variable of the system.

Alternatively, if `YARP` has been installed using the [robotology-superbuild](https://github.com/robotology/robotology-superbuild), it is possible to use `<directory-where-you-downloaded-robotology-superbuild>/build/install` as the `<installation_path>`.

## 2.3. How to run argusCamera driver


FIXME remove subdevice clauses

From command line:

```bash
yarpdev --device frameGrabber_nws_yarp --subdevice argusCamera --name /right_cam --period 0.033 --width 640 --height 480 --rotation 90.0
```

or

```
yarpdev --from argusConf.ini
```

Where `argusConf.ini`:

```ini
device frameGrabber_nws_yarp
subdevice argusCamera
name /right_cam
period 0.033
width 640
height 480
rotation 90.0
```


This is instead the minimum number of parameters for running the device, the default nws is `frameGrabber_nws_yarp`:
```
yarpdev --device argusCamera
```

# 3. Device documentation
This device driver exposes the `yarp::dev::IFrameGrabberImage` and
`yarp::dev::IFrameGrabberControls` interfaces to read the images and operate on
the available settings.
See the documentation for more details about each interface.

| YARP device name | YARP default nws        |
|:----------------:|:-----------------------:|
| `argusCamera`    | `frameGrabber_nws_yarp` |

FIXME: add parameters table from the param parser generator

**Suggested resolutions**

FIXME

|resolution|carrier|fps|
|-|-|-|
|640x480|mjpeg|30|
|1024x768|mjpeg|30|
|1920x1080|mjpeg|30 (Xavier NX) - 20 (Nano)|

# 4. Informations for developers


Maintainers
--------------
This repository is maintained by:

| | | | |
|:---:|:---:|:---:|:---:|
| [<img src="https://github.com/Nicogene.png" width="40">](https://github.com/Nicogene) | [@Nicogene](https://github.com/Nicogene) | [<img src="https://github.com/martinaxgloria.png" width="40">](https://github.com/martinaxgloria) | [@martinaxgloria](https://github.com/martinaxgloria) |
