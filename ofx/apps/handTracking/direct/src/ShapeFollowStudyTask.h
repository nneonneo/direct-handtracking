//
//  ShapeFollowStudyTask.h
//  Shape following study task - participants trace their finger through a specified shape.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "Touch.h"
#include "StudyTask.h"

class ShapeFollowStudyTask : public StudyTask {
public:	
	enum ShapeType {
		HORIZ_LINE,
		VERT_LINE,
		CIRCLE
	};

protected:
	struct Trial {
		ofPoint center;
		float extent; // length for line, radius for circle; +/- determines direction, cw/ccw
		ofPoint start, end;
	};
	vector<Trial> trials;

	struct Point {
		ofVec2f pt;
		double timestamp;
	};
	vector<Point> points;

	int touchId;

	ShapeType shapeType;

	void recordInstance();

public:
	ShapeFollowStudyTask(const string &session, const string &tag, const vector<StudyTouchTracker> &trackers, BaseApp &app, ShapeType shapeType);

	virtual void drawProjector();
	virtual void drawDebug();
	virtual void update();
	virtual int getNumTrials();
	virtual bool undoTrial();
};
