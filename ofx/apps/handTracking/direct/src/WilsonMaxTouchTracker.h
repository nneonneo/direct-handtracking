//
//  WilsonMaxTouchTracker.h
//  Implementation of Andy Wilson's background-subtraction touch tracker, using a maximum-distance background.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "WilsonTouchTracker.h"

class WilsonMaxTouchTracker : public WilsonTouchTracker {
protected:
	/* Touch tracking */
	vector<FingerTouch> findTouches();

	ofShortPixels bg; // maximum BG frame

public:
	WilsonMaxTouchTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background)
		: WilsonTouchTracker(depthStream, irStream, background) {
		bg.allocate(w, h, OF_IMAGE_COLOR_ALPHA);
	}
	virtual ~WilsonMaxTouchTracker() {
		stopThread();
		waitForThread();
	}
};
