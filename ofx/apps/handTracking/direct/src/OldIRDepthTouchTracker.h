#pragma once

#include "ofMain.h"
#include "ofxKinect2.h"
#include "ofxOpenCv.h"

#include "TouchTracker.h"

class OldIRDepthTouchTracker : public TouchTracker {
protected:
	void threadedFunction();

	/* Touch tracking stages */
	void fillIrCannyHoles();
	void touchDetectionConnectedComponents(const uint32_t *src, const uint8_t *edges, uint32_t *labels);
	vector<FingerTouch> touchTrackingConnectedComponents(uint32_t *touchpx);
	vector<FingerTouch> mergeTouches(vector<FingerTouch> &curTouches, vector<FingerTouch> &newTouches);
public:
	/* Images should not be modified outside this class */
	ofImage diffimage, blobviz, touchviz;
	ofxCvGrayscaleImage irCanny;

	/* Public methods */
	OldIRDepthTouchTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background);
	virtual ~OldIRDepthTouchTracker();

	virtual void drawDebug(float x, float y);
	virtual bool update(vector<FingerTouch> &retTouches);
};
