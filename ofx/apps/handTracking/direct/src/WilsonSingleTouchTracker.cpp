//
//  WilsonSingleTouchTracker.cpp
//  Implementation of Andy Wilson's background-subtraction touch tracker, using a single-frame background.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#include "WilsonSingleTouchTracker.h"
#include "TextUtils.h"

vector<FingerTouch> WilsonSingleTouchTracker::findTouches() {
	int n = w*h;
	uint16_t *depthPx = depthStream.getPixelsRef().getPixels();

	static int frameNumber = 0;

	/* Magic number for capturing the frame */
	if(frameNumber++ == 30) {
		memcpy(bg.getPixels(), depthPx, n*sizeof(uint16_t));
	}

	vector<FingerTouch> touches;

	doDepthThresh(bg.getPixels(), 6, 12);
	doLowpassFilter(3, 50);
	vector<ofVec2f> touchPts = findBlobs(5);
	for(ofVec2f &pt : touchPts) {
		FingerTouch touch;
		touch.tip.set(pt.x, pt.y);
		touch.touched = true;
		touches.push_back(touch);
	}

	return touches;
}
