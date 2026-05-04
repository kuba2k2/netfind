# Netfind

## Building

Before building anything (especially the Agent), make sure to clone this repository with all submodules:

```shell
git submodule update --init --recursive
```

### Netfind Agent

The Netfind Agent is a C program using CMake as its build system, so build steps are similar as with any other CMake project.

Installing `libpcap` is required prior to building; the exact steps vary between different operating systems.

The `libpcap` build process also requires `bison` and `flex`, regardless of the platform.

#### On Linux

You'll need to have GCC, CMake and Make installed.

`libpcap` also needs Linux headers (usually named `linux-headers` in most package managers).

To build the agent executable:

```shell
cd netfind-agent
cmake -S . -B cmake-build
cmake --build cmake-build
```

#### On Windows

You'll need to have GCC, CMake and Make installed - compiling using MSVC/Visual Studio is not supported, as the Agent requires support for POSIX threads.

To install `libpcap`:

1. Download the `Npcap installer` and `Npcap SDK` from [npcap.com](https://npcap.com/#download).
2. Install Npcap using the provided installer. If having problems, install in WinPcap-compatible mode.
3. Unzip the SDK somewhere, remember the path to the directory having `Lib` and `Include` subdirectories.

To build the agent executable:

```shell
cd netfind-agent
cmake -S . -B cmake-build -G "Unix Makefiles" -DPacket_ROOT=d:\path\to\npcap-sdk\
cmake --build cmake-build
```
