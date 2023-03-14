# csc369_a3

A simulation of virtual memory.

## Dependencies

This repository uses CMake to configure and build each binary.
The `teach.cs` machines have version 3.16 already installed.
But if you are working locally, you will need to install CMake as well.

## Compiling

You can use the `Makefile` provided in `src` if you would like.
Simply navigate to the `src` directory and enter `make` on the command line.
However, this will build everything in the `src` directory.
For an "out-of-source" build, you can continue to use `cmake`.
CMake can configure the project for different build systems and IDEs (type `cmake --help` for a list of generators available for your platform).

You can also work via the command line.
We recommend you create a build directory before invoking CMake to configure the project (`cmake -B`).
For example, we can perform the configuration step from the project root directory:

	cmake -H. -Bcmake-build-release -DCMAKE_BUILD_TYPE=Release
	cmake -H. -Bcmake-build-debug -DCMAKE_BUILD_TYPE=Debug

After the configuration step, you can ask CMake to build the project.

	cmake --build cmake-build-release/ --target all
	cmake --build cmake-build-debug/ --target all

## Running

- Using the Makefile: If compilation was successful, you should find the `sim` binary in your `src` directory.
- Using CMake: If compilation was successful, you will find the compiled binaries **inside your build directory** (e.g., `cmake-build-debug/src`).

Note that two successfully run the simulation, you will need to provide certain arguments on the command line:

    USAGE: sim -f tracefile -m memorysize -s swapsize -a algorithm

You can find trace files on teach.cs at: `/u/csc369h/winter/pub/a3/traces`.
To generate your own traces, see the `benchmarks` and `scripts` directories.
