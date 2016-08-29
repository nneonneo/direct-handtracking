//
//  WilsonSingleTouchTracker.h
//  Implementation of Andy Wilson's background-subtraction touch tracker, using a single-frame background.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "WilsonTouchTracker.h"

class WilsonSingleTouchTracker : public WilsonTouchTracker {
protected:
	/* Touch tracking */
	vector<FingerTouch> findTouches();

	ofShortPixels bg; // single BG frame

public:
	WilsonSingleTouchTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background)
		: WilsonTouchTracker(depthStream, irStream, background) {
		bg.allocate(w, h, OF_IMAGE_COLOR_ALPHA);
	}
	virtual ~WilsonSingleTouchTracker() {
		stopThread();
		waitForThread();
	}
};
