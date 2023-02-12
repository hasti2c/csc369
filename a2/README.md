# csc369_a2_thread

A library that enables preemptive, user-level threading.

## Dependencies

This repository uses CMake to configure and build each binary.
The `teach.cs` machines have version 3.16 already installed.
But if you are working locally, you will need to install CMake as well.

The library can be tested using the [Check unit testing framework](https://github.com/libcheck/check).
The `teach.cs` machines have version 0.15.2 installed (see `https://github.com/libcheck/check`).
If you are working locally, you will need to install this as well.
You should build from source (it is a small library, should not take long), see the library's [CMake instructions](https://github.com/libcheck/check#installing).
If you choose not to install Check, then (in the top-level `CMakeLists.txt`) you should comment out

    # add_subdirectory(tests)

## Compiling


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

If compilation was successful, you will find the compiled binaries **inside your build directory** (e.g., `cmake-build-debug`).
