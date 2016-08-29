#include "ofMain.h"

#ifndef TARGETNAME
#pragma message ("TARGETNAME undefined, using UberTest")
#define TARGETNAME UberTest
#endif

/* Preprocessor acrobatics to include the app header based on the target */
#define STRINGIZE(x) #x
#define OFAPP_INCLUDE2(x) STRINGIZE(x##_ofApp.h)
#define OFAPP_INCLUDE(x) OFAPP_INCLUDE2(x)
#include OFAPP_INCLUDE(TARGETNAME)

//========================================================================
int main( ){
	ofSetupOpenGL(3840,1080,OF_WINDOW);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}
