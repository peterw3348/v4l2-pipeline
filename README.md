
---

# V4L2 MJPEG to YUYV Userspace Conversion Pipeline

This project demonstrates a minimal userspace V4L2 pipeline that captures MJPEG frames from a webcam, decodes them into I420 using libjpeg-turbo, converts them into YUYV (YUY2), and outputs the frames to a v4l2loopback virtual device.

The project was developed and tested inside a virtual machine running Ubuntu, using a laptop’s integrated webcam passed into the VM. The webcam outputs MJPEG, and v4l2loopback was used to create a virtual video device for observing the processed output.

---

## Features

* Userspace pixel-format conversion:

  * MJPEG → I420 decode using libjpeg-turbo
  * I420 → YUYV 4:2:2 manual packing
* Full V4L2 buffer lifecycle using memory-mapped I/O:
  REQBUFS → QUERYBUF → mmap → QBUF/DQBUF → STREAMON/OFF
* Capture-only mode that dumps MJPEG frames to disk
* Capture-to-output mode that forwards converted frames to v4l2loopback
* Modular structure:

  * v4l2_helper: device setup, buffer handling, streaming
  * conversion: MJPEG decode and YUYV conversion
  * my_pipeline: example pipelines

---

## System Setup (as tested)

* Ubuntu running inside a virtual machine
* Laptop’s internal webcam passed through into the VM
* Webcam outputs MJPEG
* v4l2loopback installed for output streaming
* Tested with typical laptop webcams under Ubuntu guest environments

---

## Dependencies

Install required packages:

```bash
sudo apt update
sudo apt install libturbojpeg-dev v4l2loopback-dkms build-essential
```

Load a v4l2loopback device:

```bash
sudo modprobe v4l2loopback
```

This typically creates /dev/video2.

---

## Build Instructions

Compile the project with:

```bash
gcc -o pipeline \
    my_pipeline.c v4l2_helper.c conversion.c \
    -lturbojpeg
```

---

## Running the Pipeline

### 1. Capture-only mode

Saves MJPEG frames into the `frames/` directory.

```bash
mkdir -p frames
./pipeline /dev/video0
```

---

### 2. Capture → Convert → Output to v4l2loopback

Ensure the loopback device is loaded:

```bash
sudo modprobe v4l2loopback
```

Run the conversion pipeline:

```bash
./pipeline /dev/video0 /dev/video2
```

To view the output stream:

```bash
ffplay /dev/video2
```

or

```bash
cheese --device=/dev/video2
```

---

## Architecture Overview

The flow of data is:

1. MJPEG frames are captured from the real webcam using mmap buffers.
2. libjpeg-turbo decodes MJPEG into I420 (YUV420 planar).
3. The I420 buffer is converted manually into YUYV.
4. The converted frame is queued into a v4l2loopback device.
5. Applications can read from the virtual device as if it were a real webcam.

Diagram:

```
Laptop Webcam (MJPEG)
        |
        v
V4L2 Capture Device (/dev/video0)
        |
        |  DQBUF (mmap buffer)
        v
MJPEG Frame
        |
        v
I420 Decode (libjpeg-turbo)
        |
        v
I420 → YUYV Conversion
        |
        v
V4L2 Output Device (v4l2loopback)
        |
        v
User Applications (ffplay, OBS, cheese, etc.)
```

---

## File Overview

### v4l2_helper.c / v4l2_helper.h

Implements:

* Opening V4L2 devices
* Capability checks
* Format negotiation
* REQBUFS, QUERYBUF, mmap
* STREAMON / STREAMOFF
* Enumerating device formats

### conversion.c / conversion.h

Handles:

* Initializing libjpeg-turbo
* Decoding MJPEG into I420
* Manual I420 to YUYV pixel conversion
* Managing internal buffers

### my_pipeline.c

Contains two example pipelines:

1. Capture-only: dumps MJPEG frames to disk
2. Capture-to-output: converts MJPEG to YUYV and sends to v4l2loopback

Includes a SIGINT handler for clean termination.

---

## Notes and Common Issues

### MJPEG size vs YUYV size

MJPEG is compressed, so buffer size varies.
YUYV is uncompressed, so its size is always:

```
width * height * 2 bytes
```

This is normal and expected.

### VM considerations

Webcam passthrough inside a VM can cause:

* Reduced frame rate
* Occasional stalls
* Lower resolution availability

Using smaller resolutions (e.g., 160×120 or 320×240) improves reliability.

### v4l2loopback not appearing

Check with:

```bash
dmesg | grep v4l2
```

Reload with:

```bash
sudo modprobe v4l2loopback exclusive_caps=1
```

---

## Purpose and Learning Outcomes

This project was used to understand:

* How V4L2 memory-mapped streaming works in practice
* How MJPEG webcams behave under Linux and in virtual machines
* How to perform real-time format conversion in userspace
* How to construct a complete producer → processor → consumer video path
