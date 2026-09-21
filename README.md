# sidecar

A small C++17 application built with CMake and PortAudio.

## Prerequisites

This project is currently developed in Ubuntu/WSL. Install the compiler, CMake, and PortAudio development package:

```bash
sudo apt update
sudo apt install build-essential cmake portaudio19-dev
```

## Build

From this directory (`sidecar`), configure the project and create a separate build directory:

```bash
cmake -S . -B build
cmake --build build
```

The executable is created at `build/sidecar`.

## Run

```bash
./build/sidecar
```

The application records continuously in 512-frame blocks. The callback places
each block into a bounded queue, and the main thread maintains the latest two
seconds of audio in a rolling buffer. Once the rolling window is full, it prints
`process!` every second, representing where speech-to-text processing will be
added. If the queue fills, the oldest queued block is discarded to keep the
recording close to real time. Enter `x` and press Enter to stop recording.

## Clean Build

To remove generated build files and configure from scratch:

```bash
rm -rf build
cmake -S . -B build
cmake --build build
```
