//
//  BaseApp.cpp
//  Base class for our apps.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#include "BaseApp.h"
#include "WindowUtils.h"
#include "BackgroundUpdaterThread.h"

#include "geomConfig.h"

//--------------------------------------------------------------
void BaseApp::setup(){
	ofSetFrameRate(60);

	setupWindow();
	setupKinect();

	/* Setup worker threads */
	bgthread = new BackgroundUpdaterThread(depthStream);
	bgthread->startThread();
}

void BaseApp::setupWindow(){
	removeWindowBorder();

	ofPoint target(-PROJW, 0);
	ofSetWindowPosition(target.x, target.y);
	ofSetWindowShape(PROJW + DISPW, max(DISPH, PROJH));

	/* Adjust position to compensate for the effects of the borderless adjustment */
	ofPoint actualPos(ofGetWindowPositionX(), ofGetWindowPositionY());
	ofPoint adjTarget = target - (actualPos - target);
	ofSetWindowPosition(adjTarget.x, adjTarget.y);
}

void BaseApp::setupKinect() {
	kinect = new ofxKinect2::Device();
	kinect->setup();
	kinect->setDepthColorSyncEnabled(); // needed to create the mapper object

	if(depthStream.setup(*kinect))
		depthStream.open();

#if 0
	if (colorStream.setup(*kinect))
		colorStream.open();
#endif
	if (irStream.setup(*kinect))
		irStream.open();

	/* Wait until the Kinect is running, then do the rest of the setup. */
	uint64_t start = ofGetElapsedTimeMillis();
	while(depthStream.getPixelsRef().getWidth() == 0 || depthStream.getPixelsRef().getHeight() == 0) {
		if(ofGetElapsedTimeMillis() - start > 5000) {
			throw std::runtime_error("Kinect failed to start in time!");
		}
		ofSleepMillis(20);
	}
}

//--------------------------------------------------------------
ofPoint BaseApp::getWorldPoint(const ofVec2f &depthPos, bool live) {
	int x0 = floor(depthPos.x);
	int y0 = floor(depthPos.y);
	if(x0 < 0 || x0 >= depthStream.getWidth() - 1 || y0 < 0 || y0 >= depthStream.getHeight()-1)
		return ofPoint(0,0,0);

	/* Linearly interpolate the world point */
	ofPoint ret;
	for(int x = x0; x <= x0+1; x++) {
		for(int y = y0; y <= y0+1; y++) {
			DepthSpacePoint dpt = { x, y };

			int depth;
			int index = (int)dpt.Y * depthStream.getWidth() + (int)dpt.X;
			if(live) {
				depth = depthStream.getPixelsRef().getPixels()[index]; // current (finger) depth
			} else {
				depth = bgthread->getBackgroundMean().getPixels()[index]; // stable (background) depth
			}

			float weight = (1 - fabsf(depthPos.x - x)) * (1 - fabsf(depthPos.y - y));

			/* Map depth point to camera point */
			CameraSpacePoint wpt;
			HRESULT hr;
			hr = kinect->getMapper()->MapDepthPointToCameraSpace(dpt, depth, &wpt);
			if(!SUCCEEDED(hr)) {
				ofLogError() << "MapDepthPointToCameraSpace failed";
				return ofPoint(0, 0, 0);
			}

			ret += weight * ofPoint(wpt.X, wpt.Y, wpt.Z);
		}
	}

	return ret;
}

void BaseApp::update(){
	bgthread->update();
}

//--------------------------------------------------------------
void BaseApp::teardown() {
	/* Destroy everything cleanly. */
	delete bgthread; // destructor stops the thread for us

	irStream.stopThread();
	irStream.waitForThread();
	irStream.close();
		
#if 0
	colorStream.stopThread();
	colorStream.waitForThread();
	colorStream.close();
#endif

	depthStream.stopThread();
	depthStream.waitForThread();
	depthStream.close();

	delete kinect;
}

void BaseApp::keyPressed(int key){
	if(key == OF_KEY_ESC) {
		teardown();
	}
}

void BaseApp::keyReleased(int key){

}
