//
//  TouchBoxStudyTask.h
//  Touch box study task - participants place a specified number of fingers inside a box while the experimenter manually advances the trials.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "Touch.h"
#include "StudyTask.h"

class TouchBoxStudyTask : public StudyTask {
protected:
	struct Trial {
		ofRectangle box;
		int nFingers;
	};
	vector<Trial> trials;

	ofTrueTypeFont font;
public:
	TouchBoxStudyTask(const string &session, const string &tag, const vector<StudyTouchTracker> &trackers, BaseApp &app);

	virtual void drawProjector();
	virtual void drawDebug();
	virtual void update();
	virtual int getNumTrials();
	virtual void onKeyPressed(int key);
};
