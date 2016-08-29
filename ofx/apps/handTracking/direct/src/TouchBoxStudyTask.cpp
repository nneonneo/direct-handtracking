//
//  TouchBoxStudyTask.cpp
//  Touch box study task - participants place a specified number of fingers inside a box while the experimenter manually advances the trials.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#include "TouchBoxStudyTask.h"
#include "TextUtils.h"
#include <limits>

// XXX hardcoded stuff
static ofRectangle bounds(112, 190, 264, 40);

TouchBoxStudyTask::TouchBoxStudyTask(const string &session, const string &tag, const vector<StudyTouchTracker> &trackers, BaseApp &app)
	: StudyTask(session, tag, trackers, app) {

	int xcount = 6;
	int ycount = 1;

	for(int y=0; y<ycount; y++) {
		for(int x=0; x<xcount; x++) {
			for(int n=0; n<5; n++) {
				Trial trial;
				trial.box.set(bounds.x + bounds.width * x / xcount, bounds.y + bounds.height * y / ycount, bounds.width/xcount, bounds.height/ycount);
				trial.nFingers = n + 1;
				trials.push_back(trial);
			}
		}
	}
	random_shuffle(trials.begin(), trials.end());

	string header = "session,taskname,timestamp,trial,boxcx,boxcy,boxw,boxh,nfingers";
	for(auto &tracker : trackers) {
		header += ",";
		header += tracker.name + "_nfingers,";
		header += tracker.name + "_relerr,";
		header += tracker.name + "_abserr";
	}
	recordHeader(header);

	font.loadFont("arial.ttf", 60);
}

void TouchBoxStudyTask::drawProjector() {
	if(getCurrentTrial() >= getNumTrials())
		return;

	auto &trial = trials[getCurrentTrial()];

	ofSetLineWidth(0.004);
	ofNoFill();
	ofSetColor(255);

	ofPoint a = app.getBackgroundWorldPoint(trial.box.getTopLeft());
	ofPoint b = app.getBackgroundWorldPoint(trial.box.getTopRight());
	ofPoint c = app.getBackgroundWorldPoint(trial.box.getBottomRight());
	ofPoint d = app.getBackgroundWorldPoint(trial.box.getBottomLeft());
	ofLine(a,b);
	ofLine(b,c);
	ofLine(c,d);
	ofLine(d,a);

	ofPushMatrix();
	ofTranslate(app.getBackgroundWorldPoint(trial.box.getCenter()));
	float scale = 0.050/font.getSize(); // make text ~5cm tall
	ofScale(scale, scale, scale);
	drawText(font, ofVAArgsToString("%d", trial.nFingers), 0, 0, HAlign::center, VAlign::center);
	ofPopMatrix();
}

void TouchBoxStudyTask::drawDebug() {
}

void TouchBoxStudyTask::update() {
}

int TouchBoxStudyTask::getNumTrials() {
	return trials.size();
}

void TouchBoxStudyTask::onKeyPressed(int key) {
	if(key == ' ') {
		if(getCurrentTrial() >= getNumTrials())
			return;

		auto &trial = trials[getCurrentTrial()];

		string row = ofVAArgsToString("%s,%s,%.3f,%d,%.1f,%.1f,%.1f,%.1f,%d", getSession().c_str(), getTag().c_str(), GetTimestamp(), getCurrentTrial(),
			trial.box.getCenter().x, trial.box.getCenter().y, trial.box.width, trial.box.height, trial.nFingers);
		for(auto &tracker : trackers) {
			int inrange = 0;

			for(auto &i : tracker.touches) {
				if(!trial.box.inside(i.second.tip))
					continue;
				inrange++;
			}

			row += ofVAArgsToString(",%d,%d,%d", inrange, inrange - trial.nFingers, abs(inrange - trial.nFingers));
		}
		recordTrial(row);
	}
}
