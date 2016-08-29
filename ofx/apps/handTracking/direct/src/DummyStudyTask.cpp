//
//  DummyStudyTask.cpp
//  Dummy study task for testing purposes.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#include "DummyStudyTask.h"
#include "TextUtils.h"

DummyStudyTask::DummyStudyTask(const string &session, const string &tag, const vector<StudyTouchTracker> &trackers, BaseApp &app)
	: StudyTask(session, tag, trackers, app) {
		recordHeader("trial#,timestamp");
}

void DummyStudyTask::drawProjector() {
	/* Reproject touches */
	for(auto &t : trackers) {
		for(auto &entry : t.touches) {
			auto &touch = entry.second;
			ofPoint worldPt = app.getBackgroundWorldPoint(touch.tip);
			ofNoFill();
			ofCircle(worldPt, 0.010);
			ofDrawBitmapString(ofVAArgsToString("%.2f\n%d", touch.touchZ, touch.id), worldPt);
		}
	}
}

void DummyStudyTask::drawDebug() {
	drawText(ofVAArgsToString("current trial: %d", getCurrentTrial()), 1920, 1080, HAlign::right, VAlign::bottom);
}

void DummyStudyTask::update() {
}

int DummyStudyTask::getNumTrials() {
	return 10;
}

void DummyStudyTask::onKeyPressed(int key) {
	if(key == ' ') {
		recordTrial(ofVAArgsToString("%d,%llu", getCurrentTrial(), ofGetElapsedTimeMillis()));
	}
}
