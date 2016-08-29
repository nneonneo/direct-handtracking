//
//  BasicTest_ofApp.cpp
//  Basic functionality test app.
// 
//  Created by Robert Xiao on April 9, 2015.
//
//

#include "BasicTest_ofApp.h"
#include "TextUtils.h"

#include "geomConfig.h"

#include "IRDepthTouchTracker.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetFrameRate(60);

	BaseApp::setup();

	touchTracker = new IRDepthTouchTracker(depthStream, irStream, *bgthread);
	touchTracker->startThread();

	setupDebug();
}

void ofApp::setupDebug() {
	lastDepthTimestamp = 0;
	curDepthFrame = 0;
	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();
	depthviz.allocate(dw, dh, OF_IMAGE_GRAYSCALE);
}

void ofApp::update(){
	BaseApp::update();

	vector<FingerTouch> newTouches;
	if(touchTracker->update(newTouches)) {
		handleTouches(newTouches);
	}

	updateDebug();
}

void ofApp::handleTouches(const vector<FingerTouch> &newTouches) {
	map<int, FingerTouch> newTouchMap;		
	set<int> touchDown, touchUp;
	map<int, FingerTouch> allTouches;

	/* Sort touches */
	for(auto &i : touchMap) {
		allTouches[i.first] = i.second;
	}

	for(auto touch : newTouches) {
		newTouchMap[touch.id] = touch;
		allTouches[touch.id] = touch;

		if(!touchMap.count(touch.id) || (!touchMap[touch.id].touched && touch.touched))
			touchDown.insert(touch.id);
	}

	for(auto &i : touchMap) {
		if(!newTouchMap.count(i.first) || (i.second.touched && !newTouchMap[i.first].touched))
			touchUp.insert(i.first);
	}

	touchMap = newTouchMap;
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
	for(auto &entry : touchMap) {
		auto &touch = entry.second;
		ofPoint worldPt = getBackgroundWorldPoint(touch.tip);
		if(touch.touched) {
			ofNoFill();
			ofSetColor(0, 255, 0);
		} else {
			ofNoFill();
			ofSetColor(255, 0, 0);
		}
		ofCircle(worldPt, 0.010);
		ofDrawBitmapString(ofVAArgsToString("%.2f\n%d", touch.touchZ, touch.id), worldPt);
	}
}

void ofApp::drawDebug(){
	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();

	depthviz.draw(0, 0);
	drawText("Depth", 0, 0, HAlign::left, VAlign::top);

	bgthread->drawDebug(0, dh);
	touchTracker->drawDebug(dw, 0);

	drawText(ofVAArgsToString("FPS: %.1f\n", ofGetFrameRate())
		+ ofVAArgsToString("BG Update FPS: %.1f\n", bgthread->fps.fps)
		+ ofVAArgsToString("Touch Update FPS: %.1f\n", touchTracker->fps.fps), DISPW, 0, HAlign::right, VAlign::top);

	int debugMouseX = mouseX - PROJW;
	int debugMouseY = mouseY;
	if(0 <= debugMouseX && debugMouseX < dw && 0 <= debugMouseY && debugMouseY < dh) {
		ofVec2f pos(debugMouseX, debugMouseY);

		string description;
		ofPoint curPt = getLiveWorldPoint(pos);
		description += ofVAArgsToString("curpos: %.6f, %.6f, %.6f\n", curPt.x, curPt.y, curPt.z);

		ofPoint bgPt = getBackgroundWorldPoint(pos);
		description += ofVAArgsToString("bgpos:  %.6f, %.6f, %.6f\n", bgPt.x, bgPt.y, bgPt.z);

		drawText(description, 0, DISPH, HAlign::left, VAlign::bottom);
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
	delete touchTracker;

	BaseApp::teardown();
}

void ofApp::keyPressed(int key){
	if(key == OF_KEY_ESC) {
		teardown();
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}
