//
//  WilsonMaxTouchTracker.cpp
//  Implementation of Andy Wilson's background-subtraction touch tracker, using a maximum-distance background.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#include "WilsonMaxTouchTracker.h"
#include "TextUtils.h"

vector<FingerTouch> WilsonMaxTouchTracker::findTouches() {
	int n = w*h;
	uint16_t *depthPx = depthStream.getPixelsRef().getPixels();

	static int frameNumber = 0;

	/* Magic number for capturing the frame */
	if(frameNumber++ <= 15) {
		uint16_t *bgPx = bg.getPixels();
		for(int i=0; i<n; i++) {
			if(bgPx[i] < depthPx[i])
				bgPx[i] = depthPx[i];
		}
	}

	vector<FingerTouch> touches;

	doDepthThresh(bg.getPixels(), 8, 16);
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
