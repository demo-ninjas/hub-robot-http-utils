# Hub Robot - HTTP Utils

A basic set of tools to provide both HTTP serving and consuming capabilities to your robot.

## Available Tools

* HTTP Server
* *HTTP Client* (not implemented yet)

## Dependent Libraries

This library is dependent on the [Hub Robot Core](https://github.com/demo-ninjas/hub-robot-core) library - you *must* include that library in projects that use this library (this is automatically handled when using PlatformIO)

## PicoHTTPParser

The HTTP Server code is using the PicoHTTPParser library to parse HTTP requests.

A copy of a point in time of the parser is maintained in this repo - the source library is maintained here: https://github.com/h2o/picohttpparser


## Dev Machine Setup

Make sure you have the following installed on your machine: 

* A C/C++ compiler + standard dev tools installed (gcc, git, cmake etc...)
* PlatformIO Tools (https://platformio.org/)
* VSCode (https://code.visualstudio.com/)

Install the following VSCode Extensions: 

* C/C++ Extension pack (https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack) [This includes the C/C++ Extension]
* CMake Tools (https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
* PlatformIO IDE (https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)

Install the `Arduino Espressif32` Framework in PlatformIO and make sure an ESP32 toolchain is installed (We typically use `ESP32S3`, eg. `toolchain-xtensa-esp32s3`).

## Publish

This library is intended to be used as a dependency for a Platform.io app.

The `library.json` describes the library and dependencies to Platform.io.

increment the version number when making changes you want to be used by dependent libraries.

Currently, not published to PlatformIO, load the library dependency directly via the GitHub URL.

