# Graphic Rendering Demo

A simple hand-written application demonstrating basic rendering using C++.


![collage](collage.png)

☝️ A collage of rendered outputs using [./make_collage.sh](./make_collage.sh)

## Prerequisites

Before building the project, ensure you have the following installed:

### Windows
- Visual Studio 2019 or later with C++ development tools
- [CMake](https://cmake.org/download/) (3.10 or higher)

### macOS
Using Homebrew:
```bash
brew install cmake
brew install glfw
brew install glew
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install cmake
```

## Building the Project

Generate build files:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
```

## Running the Examples

The project includes one example:

1. Ray Casting (Ray):
```bash
./build/bin/ray
```

## Controls

- Press `Control + C` or `Ctrl + Z` to interrupt the program

## Troubleshooting

### Windows
- Ensure all dependencies are properly installed and findable by CMake
- Add vcpkg toolchain file to CMake if using vcpkg: `-DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake`

### Linux

### macOS
