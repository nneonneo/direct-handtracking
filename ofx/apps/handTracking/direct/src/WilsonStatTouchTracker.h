//
//  WilsonStatTouchTracker.h
//  Implementation of Andy Wilson's background-subtraction touch tracker, using a statistical subtraction method.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "WilsonTouchTracker.h"

class WilsonStatTouchTracker : public WilsonTouchTracker {
protected:
	/* Touch tracking */
	vector<FingerTouch> findTouches();
	void doDepthThresh(float znoise, float zlow, float zhigh);
public:
	WilsonStatTouchTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background)
		: WilsonTouchTracker(depthStream, irStream, background) {}
	virtual ~WilsonStatTouchTracker() {
		stopThread();
		waitForThread();
	}
};
