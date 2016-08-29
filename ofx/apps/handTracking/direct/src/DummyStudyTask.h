//
//  DummyStudyTask.h
//  Dummy study task for testing purposes.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "Touch.h"
#include "StudyTask.h"

class DummyStudyTask : public StudyTask {
public:
	DummyStudyTask(const string &session, const string &tag, const vector<StudyTouchTracker> &trackers, BaseApp &app);

	virtual void drawProjector();
	virtual void drawDebug();
	virtual void update();
	virtual int getNumTrials();
	virtual void onKeyPressed(int key);
};
