//
//  CrosshairStudyTask.cpp
//  Crosshair study task - participants click on a crosshair while the experimenter manually advances the trials.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#include "CrosshairStudyTask.h"
#include "TextUtils.h"
#include <limits>

// XXX hardcoded stuff
static ofRectangle bounds(112, 148, 264, 124);

CrosshairStudyTask::CrosshairStudyTask(const string &session, const string &tag, const vector<StudyTouchTracker> &trackers, BaseApp &app)
	: StudyTask(session, tag, trackers, app) {

	int xcount = 8;
	int ycount = 4;

	for(int y=0; y<ycount; y++) {
		for(int x=0; x<xcount; x++) {
			ofVec2f target(bounds.x + bounds.width * (x+.5) / (xcount), bounds.y + bounds.height * (y+.5) / (ycount));
			targets.push_back(target);
		}
	}
	random_shuffle(targets.begin(), targets.end());

	string header = "session,taskname,timestamp,trial,targetx,targety";
	for(auto &tracker : trackers) {
		header += ",";
		header += tracker.name + "_x,";
		header += tracker.name + "_y,";
		header += tracker.name + "_errx,";
		header += tracker.name + "_erry,";
		header += tracker.name + "_errmag,";
		header += tracker.name + "_count";
	}
	recordHeader(header);
}

void CrosshairStudyTask::drawProjector() {
	if(getCurrentTrial() >= getNumTrials())
		return;

	auto &pt = targets[getCurrentTrial()];

	ofSetLineWidth(0.004);
	ofNoFill();
	ofSetColor(255);
	float len = 6; // depth px

	ofLine(app.getBackgroundWorldPoint(pt - ofPoint(len, 0)),
		   app.getBackgroundWorldPoint(pt + ofPoint(len, 0)));
	ofLine(app.getBackgroundWorldPoint(pt - ofPoint(0, len)),
		   app.getBackgroundWorldPoint(pt + ofPoint(0, len)));
}

void CrosshairStudyTask::drawDebug() {
}

void CrosshairStudyTask::update() {
}

int CrosshairStudyTask::getNumTrials() {
	return targets.size();
}

static string format_float(float f) {
	if(isnan(f)) {
		return "";
	} else {
		return ofVAArgsToString("%f", f);
	}
}

void CrosshairStudyTask::onKeyPressed(int key) {
	if(key == ' ') {
		if(getCurrentTrial() >= getNumTrials())
			return;

		auto &pt = targets[getCurrentTrial()];

		string row = ofVAArgsToString("%s,%s,%f,%d,%f,%f", getSession().c_str(), getTag().c_str(), GetTimestamp(), getCurrentTrial(), pt.x, pt.y);
		for(auto &tracker : trackers) {
			FingerTouch closest;
			float closestDist = numeric_limits<float>::infinity();

			float nan = numeric_limits<float>::quiet_NaN();
			closest.tip.set(nan, nan, nan);

			int inrange = 0;

			for(auto &i : tracker.touches) {
				if(!bounds.inside(i.second.tip))
					continue;
				inrange++;
				float dist = i.second.tip.distance(pt);
				if(dist < closestDist) {
					closestDist = dist;
					closest = i.second;
				}
			}

			ofPoint err = closest.tip - pt;
			row += ",";
			row += format_float(closest.tip.x) + ",";
			row += format_float(closest.tip.y) + ",";
			row += format_float(err.x) + ",";
			row += format_float(err.y) + ",";
			row += format_float(err.length()) + ",";
			row += ofVAArgsToString("%d", inrange);
		}
		recordTrial(row);
	}
}
