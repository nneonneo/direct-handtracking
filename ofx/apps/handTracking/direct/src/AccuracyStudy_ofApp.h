#pragma once

#include "ofMain.h"
#include "BaseApp.h"
#include "Touch.h"

#include "StudyTask.h"

class ofApp : public BaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);

		vector<StudyTouchTracker> touchTrackers;

		void drawProjector();
		void drawDebug();

		void setupExperiment();
		void setupDebug();

		void updateDebug();

		void teardown();

		/* Study bits */
		vector<ofPtr<StudyTask>> tasks;
		int currentTask;
		bool intermission;
		ofTrueTypeFont font;
};
