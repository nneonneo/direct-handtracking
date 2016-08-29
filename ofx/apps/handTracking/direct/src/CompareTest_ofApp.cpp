//
//  CompareTest_ofApp.cpp
//  Side-by-side touch tracker comparison app.
// 
//  Created by Robert Xiao on April 9, 2015.
//
//

#include "CompareTest_ofApp.h"
#include "TextUtils.h"
#include "BackgroundUpdaterThread.h"

#include "geomConfig.h"

#include "IRDepthTouchTracker.h"
#include "WilsonSingleTouchTracker.h"
#include "WilsonMaxTouchTracker.h"
#include "WilsonStatTouchTracker.h"
#include "OmniTouchSausageTracker.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetFrameRate(60);
	BaseApp::setup();

	bgthread->setDynamicUpdate(false);

#define ADD_TRACKER(klass, fillcolor) { \
		TouchTrackerWrapper tracker; \
		tracker.color = fillcolor; \
		tracker.name = #klass; \
		tracker.tracker = new klass(depthStream, irStream, *bgthread); \
		tracker.tracker->startThread(); \
		touchTrackers.push_back(tracker); \
	}

	ADD_TRACKER(IRDepthTouchTracker, ofColor::limeGreen)
	ADD_TRACKER(WilsonSingleTouchTracker, ofColor::red)
	ADD_TRACKER(WilsonMaxTouchTracker, ofColor::orange)
	ADD_TRACKER(WilsonStatTouchTracker, ofColor::yellow)
	ADD_TRACKER(OmniTouchSausageTracker, ofColor::cyan)
#undef ADD_TRACKER
	setupDebug();
}

void ofApp::setupDebug() {
	debugShown = 0;
	lastDepthTimestamp = 0;
	curDepthFrame = 0;
	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();
	depthviz.allocate(dw, dh, OF_IMAGE_GRAYSCALE);
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

	updateDebug();
}

void ofApp::updateDebug() {
	/* Check if the frame is actually new */
	uint64_t curDepthTimestamp = depthStream.getFrameTimestamp();
	if(lastDepthTimestamp == curDepthTimestamp)
		return;
	lastDepthTimestamp = curDepthTimestamp;
	curDepthFrame++;

	/* Debugging */
	auto &depthPixels = depthStream.getPixelsRef();
	uint16_t *depthpx = depthPixels.getPixels();
	const int dw = depthPixels.getWidth();
	const int dh = depthPixels.getHeight();

	uint8_t *depthvizpx = depthviz.getPixels();

	/* Convert depth data for visualization purposes */
	for(int i=0; i<dw*dh; i++) {
		depthvizpx[i] = depthpx[i];
	}

	depthviz.reloadTexture();
}

//--------------------------------------------------------------
void ofApp::drawProjector(){
	/* In this function, draw points in real-world coordinates (metres) */
	ofSetLineWidth(0.002);
	
	/* Reproject touches */
	for(int i=0; i<touchTrackers.size(); i++) {
		auto &t = touchTrackers[i];
		for(auto &entry : t.touches) {
			auto &touch = entry.second;
			ofPoint worldPt = getBackgroundWorldPoint(touch.tip);
			if(debugShown == i) {
				ofFill();
			} else {
				ofNoFill();
			}
			ofSetColor(t.color);
			ofCircle(worldPt, 0.010);
			//ofDrawBitmapString(ofVAArgsToString("%.2f\n%d", touch.touchZ, touch.id), worldPt);
			ofDrawBitmapString(ofVAArgsToString("%d", i), worldPt);
		}
	}
}

void ofApp::drawDebug(){
	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();

	depthviz.draw(0, 0);
	drawText("Depth", 0, 0, HAlign::left, VAlign::top);

	bgthread->drawDebug(0, dh);
	if(debugShown >= 0 && debugShown < touchTrackers.size())
		touchTrackers[debugShown].tracker->drawDebug(dw, 0);

	const int lh = 13;
	setTextAlign(HAlign::right, VAlign::top);
	drawText(ofVAArgsToString("FPS: %.1f", ofGetFrameRate()), DISPW, 0);
	drawText(ofVAArgsToString("BG Update FPS: %.1f", bgthread->fps.fps), DISPW, lh);
	for(int i=0; i<touchTrackers.size(); i++) {
		ofSetColor(touchTrackers[i].color);
		drawText(ofVAArgsToString("%sTracker %d (%s) FPS: %.1f",
			(i == debugShown) ? "[shown] " : "",i,
			touchTrackers[i].name.c_str(), touchTrackers[i].tracker->fps.fps), DISPW, lh*(2+i));
	}
}

void ofApp::draw(){
	ofClear(64);

	/* Draw onto projector */
	ofPushMatrix();
	ofPushStyle();
	ofSetMatrixMode(OF_MATRIX_MODELVIEW);
    ofMultMatrix(projector_transpose);
	drawProjector();
	ofPopStyle();
	ofPopMatrix();

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
	} else if(key == ' ') {
		bgthread->captureBackground();
	} else if(key >= '0' && key <= '9') {
		debugShown = key - '0';
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}
