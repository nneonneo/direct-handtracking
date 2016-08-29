//
//  WilsonStatTouchTracker.cpp
//  Implementation of Andy Wilson's background-subtraction touch tracker, using a statistical subtraction method.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#include "WilsonStatTouchTracker.h"
#include "TextUtils.h"

vector<FingerTouch> WilsonStatTouchTracker::findTouches() {
	vector<FingerTouch> touches;

	doDepthThresh(2.0, 4.0, 20);
	doLowpassFilter(3, 100);
	vector<ofVec2f> touchPts = findBlobs(5);
	for(ofVec2f &pt : touchPts) {
		FingerTouch touch;
		touch.tip.set(pt.x, pt.y);
		touch.touched = true;
		touches.push_back(touch);
	}

	return touches;
}

void WilsonStatTouchTracker::doDepthThresh(float znoise, float zlow, float diffhigh) {
	int n = w*h;
	
	const float *bgmean = background.getBackgroundMean().getPixels();
	const float *bgstdev = background.getBackgroundStdev().getPixels();

	uint16_t *depthPx = depthStream.getPixelsRef().getPixels();
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	for(int i=0; i<n; i++) {
		float diff = bgmean[i] - depthPx[i];
		float z = diff / bgstdev[i];
		if(z < znoise) {
			blobPx[i] = 0xff000000;
		} else if(z < zlow) {
			blobPx[i] = 0xff808000;
		} else if(diff < diffhigh) {
			blobPx[i] = 0xffffff00;
		} else {
			blobPx[i] = 0xff000000;
		}
	}
}
