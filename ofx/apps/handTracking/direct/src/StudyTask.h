//
//  StudyTask.h
//  Base class for study tasks.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "Touch.h"
#include "BaseApp.h"

#include <cstdio>

struct StudyTouchTracker {
	string name;
	map<int, FingerTouch> touches;
	class TouchTracker *tracker;
};

class StudyTask {
	string tag;
	string session;
	FILE *recf; // want to be able to ftruncate
	vector<long> trialRows; // offsets within the file to each row

	int currentTrial;

protected:
	const vector<StudyTouchTracker> &trackers;
	BaseApp &app;

	void recordHeader(const string &header);
	void recordTrial(const string &row);
	bool unrecordTrial();

public:
	static string GetTimestampString();
	static double GetTimestamp();

	StudyTask(const string &session, const string &tag, const vector<StudyTouchTracker> &trackers, BaseApp &app);

	int getCurrentTrial() const { return currentTrial; }
	const string &getSession() const { return session; }
	const string &getTag() const { return tag; }

	virtual void drawProjector() = 0;
	virtual void drawDebug() = 0;
	virtual void update() = 0;
	virtual int getNumTrials() = 0;

	virtual void onKeyPressed(int key) {}
	virtual bool undoTrial() { return unrecordTrial(); }
};
