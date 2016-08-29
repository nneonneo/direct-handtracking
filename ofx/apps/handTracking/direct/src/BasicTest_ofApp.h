#pragma once

#include "ofMain.h"
#include "BaseApp.h"
#include "Touch.h"

class ofApp : public BaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);

		class TouchTracker *touchTracker;

		map<int, FingerTouch> touchMap;

		void drawProjector();
		void drawDebug();

		void setupDebug();

		void handleTouches(const vector<FingerTouch> &newTouches);
		void updateDebug();

		void teardown();

		/* Debugging */
		ofImage depthviz;
		uint64_t lastDepthTimestamp;
		int curDepthFrame;
};
