//
//  WorldKitTouchTracker.h
//  Implementation of WorldKit-style statistical BG subtraction touch tracker.
//
//  Created by Robert Xiao on April 9, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "ofxKinect2.h"
#include "ofxOpenCv.h"

#include "TouchTracker.h"

class WorldKitTouchTracker : public TouchTracker {
protected:
	void threadedFunction();

	/* Touch tracking */
	vector<FingerTouch> findTouches();
	void buildDiffImage();
	vector<ofVec2f> findBlobs();

	vector<FingerTouch> mergeTouches(vector<FingerTouch> &curTouches, vector<FingerTouch> &newTouches);
private:
	/* Double-buffered images for display's sake */
	int front;
	ofImage diffIm[2]; // diff image; B=absdiff G=type R=reldiff(=mag)
	ofImage blobIm[2]; // blob image; B=index G=indexcolor R=flags

public:
	WorldKitTouchTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background);
	virtual ~WorldKitTouchTracker();

	virtual void drawDebug(float x, float y);
	virtual bool update(vector<FingerTouch> &retTouches);
};
