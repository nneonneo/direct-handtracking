#pragma once

#include "ofMain.h"
#include "ofxKinect2.h"

class BaseApp : public ofBaseApp{

	public:
		void setup();
		void update();

		ofxKinect2::Device* kinect;
		ofxKinect2::IrStream irStream;
		ofxKinect2::ColorStream colorStream;
		ofxKinect2::DepthStream depthStream;
		class BackgroundUpdaterThread *bgthread;

		ofPoint getWorldPoint(const ofVec2f &depthPt, bool live);
		ofPoint getBackgroundWorldPoint(const ofVec2f &depthPt) { return getWorldPoint(depthPt, false); }
		ofPoint getLiveWorldPoint(const ofVec2f &depthPt) { return getWorldPoint(depthPt, true); }

		void setupWindow();
		void setupKinect();
		
		virtual void teardown();

		void keyPressed(int key);
		void keyReleased(int key);
};
