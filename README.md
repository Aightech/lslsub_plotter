# lslsub_plotter
C++ that enables to select and visualize LSL streams.


## TODO
- [X] create repository
- [ ] read qt doc
- [ ] make a gui
- [ ] scan LSL streams
- [ ] get the streams
- [ ] plot the stream
  - [ ] heatmap
  - [ ] lines

## Architecture
### INPUTS:
- LSL stream
- User selection
### OUTPUTS:
- scatter plots
- heatmap plot


## Installation
### Ubuntu 18
#### Requirements
#### Steps

### Windows 10
#### Requirements
- [CMake](https://cmake.org/download/) (download and execute the installer for windows , add to the PATH variable)
- [MinGW32](https://sourceforge.net/projects/mingw-w64/) (download and execute the installer for windows, chose i686_64 architecture, add the the bin folder address of minGW to the PATH environement variable) 
#### Steps
- Create a build directory.
- Configure and generate the CMake project.
- Build the project.
```bash
mkdir build && cd build && cmake .. -G "MinGW Makefiles" && mingw32-make
```
