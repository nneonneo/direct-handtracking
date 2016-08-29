//
//  OmniTouchSausageTracker.h
//  Implementation of OmniTouch's sausage-finding touch tracker.
// 
//  Created by Robert Xiao on April 9, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "ofxKinect2.h"
#include "ofxOpenCv.h"

#include "TouchTracker.h"

class OmniTouchSausageTracker : public TouchTracker {
protected:
	void threadedFunction();

	/* Touch tracking */
	vector<FingerTouch> findTouches();
	vector<FingerTouch> filterTouches(vector<FingerTouch> &touches);

	vector<FingerTouch> mergeTouches(vector<FingerTouch> &curTouches, vector<FingerTouch> &newTouches);
private:
	/* Double-buffered images for display's sake */
	int front;
	ofImage diffIm[2]; // diff image; B=dx G=??? R=dy
	ofImage sausageIm[2]; // sausage image; B=x G=flags R=y

public:
	OmniTouchSausageTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background);
	virtual ~OmniTouchSausageTracker();

	virtual void drawDebug(float x, float y);
	virtual bool update(vector<FingerTouch> &retTouches);
};
