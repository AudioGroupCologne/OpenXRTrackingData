Application to receive tracking information from VR System

This program is designed to fetch the tracking information of HTC VIVE Tracker devices using the openXR API.
The tracking information then will be send via the OSC Message protocol.

Prerequirements:
- Steam VR
- Cmake
- OpenXR SDK (not sure, since the .lib should be included with this project)
- DirectX 11


Usage:
The application can be build using Cmake, either directly or through VSCode (C/C++ and CMake Tools extensions needed)
It is intended to build in Release Mode, to be able to build in Debug Mode
```
target_link_libraries(RecievePositionData PRIVATE "${CMAKE_SOURCE_DIR}/openxr/lib/openxr_loader.lib"
```

has to be changed to 
```
target_link_libraries(RecievePositionData PRIVATE "${CMAKE_SOURCE_DIR}/openxr/lib/openxr_loaderd.lib"
```

in the CmakeLists.txt

The easiest way to compile is using cmake, in a terminal window navigate to the project folder and then:
```
mkdir build
cd .\build\
cmake ..
cmake --build . --config Release

```

before starting the application, the following should be checked:
- in the SteamVR Settings --> OpenXR: all OpenXR API Layers should be turned off
- at least one Tracker is powered on and has a role assigned
- SteamVR is running and active (not in Stand-By)

  The compiled .exe can then be run via the Powershell/ Terminal and immediately start sending tracking positions as OSC messgaes, if available.

  It is possible to pass arguments to the applicatio when starting
  - **-ip** defines the address to which the OSC messages are send
  - **-p** changes the port to which OSC Messages are send
  - **-m** selects the struture of the OSC Messages
    
    available message types:
    - **quaternion** : position and orientation quaternion (x,y,z, q.w, q.x, q.y, q.z)
    - **xyzpry** : position and pitch roll yaw (x,y,z, pitch, roll, yaw)
    - **pybinsim** : (/pyBinSim 0, yaw, 0,0,0,0,0,0 )
    - **xyzazel** : position and Azimuth an Elevation angle

if nothing is passed a default
**127.0.0.1:7000**  **xyzazel**
is used
It is advised not to use the Pitch, Roll, Yaw Message

  
