//
//  ShapeFollowStudyTask.cpp
//  Shape following study task - participants trace their finger through a specified shape.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#include "ShapeFollowStudyTask.h"
#include "TextUtils.h"
#include <limits>

// XXX hardcoded stuff
static ofRectangle bounds(112, 148, 264, 124);
static ofPoint shapeCenter(240, 190);

ShapeFollowStudyTask::ShapeFollowStudyTask(const string &session, const string &tag, const vector<StudyTouchTracker> &trackers, BaseApp &app, ShapeType shapeType)
	: StudyTask(session, tag, trackers, app), shapeType(shapeType) {

		float sizes[] = { -20, -15, -10, 10, 15, 20 };

		for(int i=0; i<(sizeof(sizes)/sizeof(sizes[0])); i++) {
			for(int j=0; j<2; j++) {
				Trial trial;
				trial.center = shapeCenter;
				trial.extent = sizes[i];
				switch(shapeType) {
				case HORIZ_LINE:
					trial.start = trial.center + ofPoint(sizes[i], 0);
					trial.end = trial.center - ofPoint(sizes[i], 0);
					break;
				case VERT_LINE:
					trial.start = trial.center + ofPoint(0, sizes[i]);
					trial.end = trial.center - ofPoint(0, sizes[i]);
					break;
				case CIRCLE: // always start at 0 degrees
					trial.start = trial.end = trial.center + ofPoint(abs(sizes[i]), 0);
					break;
				}
				trials.push_back(trial);
			}
		}

		random_shuffle(trials.begin(), trials.end());

		string header = "session,taskname,timestamp,trial,cx,cy,sx,sy,ex,ey,size";
		header += ",ptidx,x,y,err,abserr";
		recordHeader(header);

		touchId = -1;
}

void ShapeFollowStudyTask::drawProjector() {
	if(getCurrentTrial() >= getNumTrials())
		return;

	auto &trial = trials[getCurrentTrial()];

	ofSetLineWidth(5);
	ofNoFill();
	ofSetColor(255);

	float startDirection;

	if(shapeType == CIRCLE) {
		startDirection = 90;
		for(int i=0; i<72; i++) {
			ofPoint p(trial.extent, 0);
			ofLine(app.getBackgroundWorldPoint(p.rotated(0, 0, i*5) + trial.center),
				   app.getBackgroundWorldPoint(p.rotated(0, 0, (i+1)*5) + trial.center));
		}
	} else {
		ofLine(app.getBackgroundWorldPoint(trial.start),
			app.getBackgroundWorldPoint(trial.end));
		if(shapeType == HORIZ_LINE) {
			startDirection = 180;
		} else {
			startDirection = 90;
		}
	}
	if(trial.extent < 0)
		startDirection += 180;

	ofPushMatrix();
	ofTranslate(app.getBackgroundWorldPoint(trial.start));
	ofRotateZ(startDirection);
	ofFill();
	ofSetColor(0, 228, 0);
	ofTriangle(-0.012, -0.012,  -0.012, 0.012,  0.012, 0);
	ofPopMatrix();

	ofSetLineWidth(3);
	ofSetColor(255, 127, 127);
	for(int i=1; i<points.size(); i++) {
		ofLine(app.getBackgroundWorldPoint(points[i-1].pt),
			app.getBackgroundWorldPoint(points[i].pt));
	}
}

void ShapeFollowStudyTask::drawDebug() {
}

void ShapeFollowStudyTask::update() {
	if(getCurrentTrial() >= getNumTrials())
		return;

	auto &trial = trials[getCurrentTrial()];

	if(touchId == -1) {
		for(auto &i : trackers[0].touches) {
			if(i.second.tip.distance(trial.start) < 10) {
				touchId = i.first;
				points.clear();
				break;
			}
		}
	}

	if(touchId != -1) {
		auto i = trackers[0].touches.find(touchId);
		if(i != trackers[0].touches.end()) {
			// touch move
			if(!i->second.missing) {
				Point pt;
				pt.pt = i->second.tip;
				pt.timestamp = GetTimestamp();
				points.push_back(pt);
			}
		} else {
			// touch up: record
			recordInstance();
			points.clear();
			touchId = -1;
		}
	}
}

int ShapeFollowStudyTask::getNumTrials() {
	return trials.size();
}

void ShapeFollowStudyTask::recordInstance() {
	if(getCurrentTrial() >= getNumTrials())
		return;

	auto &trial = trials[getCurrentTrial()];

	string row = "";

	for(int i=0; i<points.size(); i++) {
		auto &pt = points[i];
		float err;

		switch(shapeType) {
		case HORIZ_LINE:
			err = pt.pt.y - trial.center.y;
			break;
		case VERT_LINE:
			err = pt.pt.x - trial.center.x;
			break;
		case CIRCLE:
			err = pt.pt.distance(trial.center) - abs(trial.extent);
			break;
		}

		row += ofVAArgsToString("%s,%s,%f,%d,%f,%f,%f,%f,%f,%f,%f", getSession().c_str(), getTag().c_str(), pt.timestamp, getCurrentTrial(),
			trial.center.x, trial.center.y, trial.start.x, trial.start.y, trial.end.x, trial.end.y, trial.extent);
		row += ofVAArgsToString(",%d,%f,%f,%f,%f", i, pt.pt.x, pt.pt.y, err, abs(err));
		if(i != points.size()-1)
			row += "\n";
	}

	recordTrial(row);
}

bool ShapeFollowStudyTask::undoTrial() {
	if(StudyTask::undoTrial()) {
		points.clear();
		touchId = -1;
		return true;
	}
	return false;
}
