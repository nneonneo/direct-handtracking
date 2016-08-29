#pragma once

#include "ofMain.h"
#include "BaseApp.h"
#include "Touch.h"

struct TouchTrackerWrapper {
	string name;
	map<int, FingerTouch> touches;
	ofColor color;
	class TouchTracker *tracker;
};

class ofApp : public BaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);

		vector<TouchTrackerWrapper> touchTrackers;

		void drawProjector();

		void drawDebug();
		void setupDebug();
		void updateDebug();

		void teardown();

		/* Debugging */
		ofImage depthviz;
		uint64_t lastDepthTimestamp;
		int curDepthFrame;
		int debugShown;
};
