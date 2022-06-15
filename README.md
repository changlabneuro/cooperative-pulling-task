This repository contains the source code for the cooperative lever-pulling task.

### build + run

1. Clone the repository: `git clone --recursive https://github.com/omeisner/om-lever-task`. Note the `--recursive` flag.
2. On Windows:
   1. Visual Studio 2022 is recommended - the community version is free to download.
   2. Open Visual Studio and select "open a local folder", then open the repository directory (the `om-lever-task` directory).
   3. In the top menu bar, click "select start-up item" and select a build target. Currently, the main task is called `test_gui_context`. To the left of the startup item, change the build configuration from `x64-Debug` to `x64-Release`.
   4. Click the run button to build and run the program.
3. On macOS:
   1. Open a terminal window and type `cmake -version`. If you get an error indicating cmake is not found, or if the version is less than `3.5`, [download and install cmake](https://cmake.org/download/).
   2. `cd` to the repository directory, e.g. `cd om-lever-task`
   3. Make a directory called build: `mkdir build && cd build`
   4. Generate build files: `cmake -DCMAKE_BUILD_TYPE=Release -G Xcode ..`
   5. Build a program. Currently, the main task is called `test_gui_context`:
      1. `cmake --build . --target test_gui_context --config Release`
   6. `cd` to the build output directory and run the program: 
      1. `cd src/sandbox/test_gui_context/Release`
      2. `./test_gui_context`