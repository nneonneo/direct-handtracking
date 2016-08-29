//
//  CrosshairStudyTask.h
//  Crosshair study task - participants click on a crosshair while the experimenter manually advances the trials.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "Touch.h"
#include "StudyTask.h"

class CrosshairStudyTask : public StudyTask {
protected:
	vector<ofVec2f> targets;

public:
	CrosshairStudyTask(const string &session, const string &tag, const vector<StudyTouchTracker> &trackers, BaseApp &app);

	virtual void drawProjector();
	virtual void drawDebug();
	virtual void update();
	virtual int getNumTrials();
	virtual void onKeyPressed(int key);
};
