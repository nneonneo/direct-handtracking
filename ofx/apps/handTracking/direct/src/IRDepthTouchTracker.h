//
//  IRDepthTouchTracker.h
//  Infrared + Depth sensor-fusion touch tracker.
// 
//  Created by Robert Xiao on April 8, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "ofxKinect2.h"
#include "ofxOpenCv.h"

#include "TouchTracker.h"

struct IRDepthTip {
	vector<unsigned> pixels;
	vector<unsigned> roots; // pixels next to midconf/highconf pixels
};

struct IRDepthFinger {
	float x, y, z;
};

struct IRDepthHand {
	vector<IRDepthFinger> fingers;
};

struct IRDepthArm {
	vector<IRDepthHand> hands;
};

class IRDepthTouchTracker : public TouchTracker {
protected:
	void threadedFunction();

	/* Edgemap construction */
	void buildEdgeImage();
	// Fill holes in the irCanny image
	void fillIrCannyHoles();

	/* Touch tracking stages */
	void buildDiffImage();

	void rejectBlob(const vector<unsigned> &blob, int reason=0);

	int nextBlobId;
	vector<IRDepthArm> detectTouches();
	bool floodArm(IRDepthArm &arm, unsigned idx);
	bool floodHand(IRDepthHand &hand, unsigned idx);
	bool floodFinger(IRDepthFinger &finger, unsigned idx);
	bool floodTip(IRDepthTip &tip, unsigned idx);
	void refloodFinger(const vector<unsigned> &blob, vector<unsigned> &roots);
	bool computeFingerMetrics(IRDepthFinger &finger, vector<unsigned> &px);

	vector<FingerTouch> mergeTouches(vector<FingerTouch> &curTouches, vector<FingerTouch> &newTouches);
private:
	/* Double-buffered images for display's sake */
	int front;
	ofImage diffIm[2]; // depth difference image; A=valid B=zone [0=noise/negative 64=close 128=medium 192=far] GR=diff
	ofImage edgeIm[2]; // edge image; B=IRedge G=depthedge R=depthabs
	ofImage blobIm[2]; // blob image; B=flags G=blobidx R=dist
	ofxCvGrayscaleImage irCanny; // temporary image for canny purposes

public:
	IRDepthTouchTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background);
	virtual ~IRDepthTouchTracker();

	virtual void drawDebug(float x, float y);
	virtual bool update(vector<FingerTouch> &retTouches);
};
