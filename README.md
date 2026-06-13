# Netfind

## Building

Before building anything (especially the Agent), make sure to clone this repository with all submodules:

```shell
git submodule update --init --recursive --depth=1
```

### Netfind Agent

The Netfind Agent is a C program using CMake as its build system, so build steps are similar as with any other CMake project.

Installing `libpcap` is required prior to building; the exact steps vary between different operating systems.

The `libpcap` build process also requires `bison`, `flex` and `protoc`, regardless of the platform. Your system-wide Python interpreter should have the `protobuf` package installed.

#### On Linux

You'll need to have GCC, CMake and Make installed.

`libpcap` also needs Linux headers (usually named `linux-headers` in most package managers).

To build and run the agent executable:

```shell
cd netfind-agent
cmake -S . -B cmake-build
cmake --build cmake-build -t netfind-agent
# run the Agent - as root to enable packet capture
sudo cmake-build/src/netfind-agent
```

#### On Windows

You'll need to have GCC, CMake and Make installed - compiling using MSVC/Visual Studio is not supported, as the Agent requires support for POSIX threads.

To install `libpcap`:

1. Download the `Npcap installer` and `Npcap SDK` from [npcap.com](https://npcap.com/#download).
2. Install Npcap using the provided installer. If having problems, install in WinPcap-compatible mode.
3. Unzip the SDK somewhere, remember the path to the directory having `Lib` and `Include` subdirectories.

To build and run the agent executable:

```shell
cd netfind-agent
cmake -S . -B cmake-build -G "Unix Makefiles" -DPacket_ROOT=d:\path\to\npcap-sdk\
cmake --build cmake-build -t netfind-agent
# run the Agent
cmake-build\src\netfind-agent.exe
```

### Netfind Server

The Netfind Server is a Python program, not using any packaging system yet.

It's recommended to run in a virtual environment when deploying Netfind to an embedded device or a server. The following steps should work on Linux and Windows (after adjusting the `activate` command).

To configure and run the server:

```shell
cd netfind-server
python -m venv .venv
# activate venv - for Linux:
. .venv/bin/activate
# activate venv - for Windows:
.venv\Scripts\activate.bat
# install dependencies
pip install flask flask_sock pure-protobuf
# initialize the database (WARNING: will delete the old one, if present)
flask --app nfserver init-db
# run the Server
flask --app nfserver run --host=0.0.0.0 --port=6346
```
