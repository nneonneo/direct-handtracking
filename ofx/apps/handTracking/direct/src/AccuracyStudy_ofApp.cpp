//
//  AccuracyStudy_ofApp.cpp
//  Experimental study app.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#include "AccuracyStudy_ofApp.h"
#include "TextUtils.h"
#include "BackgroundUpdaterThread.h"

#include "geomConfig.h"

#include "IRDepthTouchTracker.h"
#include "WilsonSingleTouchTracker.h"
#include "WilsonMaxTouchTracker.h"
#include "WilsonStatTouchTracker.h"
#include "OmniTouchSausageTracker.h"

#include "CrosshairStudyTask.h"
#include "TouchBoxStudyTask.h"
#include "ShapeFollowStudyTask.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetFrameRate(60);
	BaseApp::setup();

	bgthread->setDynamicUpdate(false);

#define ADD_TRACKER(klass) { \
		StudyTouchTracker tracker; \
		tracker.name = #klass; \
		tracker.tracker = new klass(depthStream, irStream, *bgthread); \
		tracker.tracker->startThread(); \
		touchTrackers.push_back(tracker); \
	}

	ADD_TRACKER(IRDepthTouchTracker)
	ADD_TRACKER(WilsonSingleTouchTracker)
	ADD_TRACKER(WilsonMaxTouchTracker)
	ADD_TRACKER(WilsonStatTouchTracker)
	ADD_TRACKER(OmniTouchSausageTracker)
#undef ADD_TRACKER

	setupDebug();
	setupExperiment();
}

void ofApp::setupExperiment() {
	string session = StudyTask::GetTimestampString();
	tasks.push_back(ofPtr<StudyTask>(new CrosshairStudyTask(session, "ch_front", touchTrackers, *this)));
	tasks.push_back(ofPtr<StudyTask>(new CrosshairStudyTask(session, "ch_back", touchTrackers, *this)));
	tasks.push_back(ofPtr<StudyTask>(new CrosshairStudyTask(session, "ch_front2", touchTrackers, *this)));
	tasks.push_back(ofPtr<StudyTask>(new CrosshairStudyTask(session, "ch_back2", touchTrackers, *this)));
	tasks.push_back(ofPtr<StudyTask>(new TouchBoxStudyTask(session, "tb_front", touchTrackers, *this)));
	tasks.push_back(ofPtr<StudyTask>(new TouchBoxStudyTask(session, "tb_back", touchTrackers, *this)));
	tasks.push_back(ofPtr<StudyTask>(new TouchBoxStudyTask(session, "tb_front2", touchTrackers, *this)));
	tasks.push_back(ofPtr<StudyTask>(new TouchBoxStudyTask(session, "tb_back2", touchTrackers, *this)));
	tasks.push_back(ofPtr<StudyTask>(new ShapeFollowStudyTask(session, "sh_hline_back", touchTrackers, *this, ShapeFollowStudyTask::HORIZ_LINE)));
	tasks.push_back(ofPtr<StudyTask>(new ShapeFollowStudyTask(session, "sh_vline_back", touchTrackers, *this, ShapeFollowStudyTask::VERT_LINE)));
	tasks.push_back(ofPtr<StudyTask>(new ShapeFollowStudyTask(session, "sh_circle_back", touchTrackers, *this, ShapeFollowStudyTask::CIRCLE)));
	tasks.push_back(ofPtr<StudyTask>(new ShapeFollowStudyTask(session, "sh_hline_front", touchTrackers, *this, ShapeFollowStudyTask::HORIZ_LINE)));
	tasks.push_back(ofPtr<StudyTask>(new ShapeFollowStudyTask(session, "sh_vline_front", touchTrackers, *this, ShapeFollowStudyTask::VERT_LINE)));
	tasks.push_back(ofPtr<StudyTask>(new ShapeFollowStudyTask(session, "sh_circle_front", touchTrackers, *this, ShapeFollowStudyTask::CIRCLE)));

	random_shuffle(tasks.begin(), tasks.end());

	currentTask = 0;
	intermission = true;
	font.loadFont("arial.ttf", 180);
}

//--------------------------------------------------------------
void ofApp::update(){
	BaseApp::update();

	for(auto &t : touchTrackers) {
		vector<FingerTouch> newTouches;
		if(t.tracker->update(newTouches)) {
			t.touches.clear();
			for(FingerTouch &touch : newTouches) {
				t.touches[touch.id] = touch;
			}
		}
	}

	if(!intermission && currentTask < tasks.size()) {
		tasks[currentTask]->update();
		if(tasks[currentTask]->getCurrentTrial() >= tasks[currentTask]->getNumTrials()) {
			currentTask++;
			intermission = true;
		}
	}

	updateDebug();
}

//--------------------------------------------------------------
void ofApp::drawProjector(){
	/* In this function, draw points in real-world coordinates (metres) */
	ofSetLineWidth(0.002);

	if(!intermission && currentTask < tasks.size())
		tasks[currentTask]->drawProjector();
}

void ofApp::setupDebug() {
}

void ofApp::updateDebug() {
}

void ofApp::drawDebug(){
	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();

	depthStream.draw();
	drawText("Depth", 0, 0, HAlign::left, VAlign::top);

	if(!intermission && currentTask < tasks.size())
		tasks[currentTask]->drawDebug();

	string msg = ofVAArgsToString("FPS: %.1f\n", ofGetFrameRate())
		+ ofVAArgsToString("BG Update FPS: %.1f\n", bgthread->fps.fps);

	for(int i=0; i<touchTrackers.size(); i++) {
		msg += ofVAArgsToString("Tracker %d FPS: %.1f\n", i, touchTrackers[i].tracker->fps.fps);
	}
	
	msg += "\n";
	if(currentTask < tasks.size()) {
		auto &task = tasks[currentTask];
		msg += ofVAArgsToString("Task: %d/%d: %s\n", currentTask+1, tasks.size(), task->getTag().c_str());
		msg += ofVAArgsToString("Trial: %d/%d\n", task->getCurrentTrial()+1, task->getNumTrials());
	} else {
		msg += ofVAArgsToString("Task: ended (%d total)\n", tasks.size());
	}

	drawText(msg, DISPW, 0, HAlign::right, VAlign::top);
}

void ofApp::draw(){
	ofClear(0);

	/* Draw onto projector */
	ofPushMatrix();
	ofPushStyle();
	ofSetMatrixMode(OF_MATRIX_MODELVIEW);
    ofMultMatrix(projector_transpose);
	drawProjector();
	ofPopStyle();
	ofPopMatrix();
	if(intermission && currentTask < tasks.size()) {
		drawText(font, tasks[currentTask]->getTag(), PROJW/2, PROJH/2, HAlign::center, VAlign::center);
	}

	/* Draw debug info */
	ofPushMatrix();
	ofPushStyle();
	ofTranslate(PROJW, 0);
	drawDebug();
	ofPopStyle();
	ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::teardown() {
	/* Destroy everything cleanly. */
	for(auto &t : touchTrackers) {
		delete t.tracker;
	}
	BaseApp::teardown();
}

void ofApp::keyPressed(int key){
	if(key == OF_KEY_ESC) {
		teardown();
	} else if(key == OF_KEY_BACKSPACE) {
		if(currentTask >= tasks.size())
			currentTask = tasks.size()-1;
		while(1) {
			if(tasks[currentTask]->undoTrial())
				break;
			else if(currentTask == 0)
				break;
			else
				currentTask--;
		}
	} else if(intermission) {
		intermission = !intermission;
	} else if(currentTask < tasks.size()) {
		tasks[currentTask]->onKeyPressed(key);
		if(tasks[currentTask]->getCurrentTrial() >= tasks[currentTask]->getNumTrials()) {
			currentTask++;
			intermission = true;
		}
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}
