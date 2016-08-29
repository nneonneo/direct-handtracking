//
//  WilsonTouchTracker.h
//  Abstract implementation of Andy Wilson's background-subtraction touch tracker.
// 
//  Created by Robert Xiao on April 9, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "ofxKinect2.h"
#include "ofxOpenCv.h"

#include "TouchTracker.h"

class WilsonTouchTracker : public TouchTracker {
protected:
	void threadedFunction();

	/* Touch tracking */
	virtual vector<FingerTouch> findTouches() = 0;
	void doDepthThresh(const uint16_t *bgPx, int tlow, int thigh);
	static void boxcarFilterH(unsigned char *pixels, int w, int h, int channel, int filtersz);
	static void boxcarFilterV(unsigned char *pixels, int w, int h, int channel, int filtersz);
	void doLowpassFilter(int filtersz, int thresh);
	vector<ofVec2f> findBlobs(int minsize);

	vector<FingerTouch> mergeTouches(vector<FingerTouch> &curTouches, vector<FingerTouch> &newTouches);

	/* Double-buffered images for display's sake */
	int front;
	ofImage blobIm[2]; // blob image; B=zone G=smoothed R=thresholded

public:
	WilsonTouchTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background);

	virtual void drawDebug(float x, float y);
	virtual bool update(vector<FingerTouch> &retTouches);
};
