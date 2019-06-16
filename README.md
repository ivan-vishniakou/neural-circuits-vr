# Virtual reality application for neuroscience experiments

## About
This software was developed at Center of Advanced European Studies and Research (caesar) for behavior experiments on fruit flies. See [BioRxiv preprint](https://www.biorxiv.org/content/10.1101/579813v1) for details.

## Dependencies
3Rd party libraries are listed with versions the project was built with:
 - [OpenCV 4.0.1](https://opencv.org/releases.html)
Make sure ```OPENCV_DIR``` is set in the environment variables, as well as the location of dlls are added to ```PATH```
 - [pylon 5.1.0 Camera Software Suite Windows](https://www.baslerweb.com/en/products/software/basler-pylon-camera-software-suite/)
Developer version should be installed which sets the ```PYLON_DEV_DIR``` environment variable
- [Urho3D 1.7](https://urho3d.github.io/)
Pointed at with ```URHO3D_DIR``` environment variable
 - [Boost 1.69.0 msvc-14.1-64](https://sourceforge.net/projects/boost/files/boost-binaries/1.69.0/)
Pointed at with ```BOOST_DIR``` environment variable
- [Spdlog 1.3.1](https://github.com/gabime/spdlog/releases)
Pointed at with ```SPDLOG_DIR``` environment variable

## Binaries
Compiled executable can be found [here](https://www.dropbox.com/s/yhlz9kb869752cj/nc-vr.zip?dl=0).
