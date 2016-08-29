## DIRECT - Depth IR Enhanced Contact Tracking

This is the source code to accompany the academic paper on DIRECT - Depth IR Enhanced Contact Tracking.

## Prerequisites

You'll need a computer running Windows 8.1 or above, and a Kinect 2 depth sensor. Your computer will need a USB 3 port capable of interfacing with the Kinect.

## Building

The source code is designed for use with the [openFrameworks](http://openframeworks.cc/) C++ framework and the Microsoft Windows operating system.

Install the following software components:

- [Visual Studio 2012](https://www.microsoft.com/en-us/download/details.aspx?id=34673)
- [Kinect 2 SDK](https://www.microsoft.com/en-us/download/details.aspx?id=44561)
- [Awesomium](http://www.awesomium.com/download)

Download [openFrameworks 0.8.4 for Visual Studio 2012](http://www.openframeworks.cc/versions/v0.8.4/of_v0.8.4_vs_release.zip) and unpack it to the `ofx` directory here (such that the unpacked `apps` directory and this repository's `apps` directory coincide). Clone the [ofxKinect2](https://github.com/sadmb/ofxKinect2) repository into the `addons` directory.

Now just open [direct.sln](ofx/apps/handTracking/direct/direct.sln). There are several configurations corresponding to separate apps:

- AccuracyStudy: Code used to conduct the user study
- BasicTest: Basic functionality test
- CompareTest: Side-by-side comparison of different techniques
- UberTest: Paint demo

