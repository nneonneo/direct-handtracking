//
//  UberTest_ofApp.cpp
//  All-inclusive feature test app.
//
//  Created by Robert Xiao on January 28, 2015.
//
//

#include "UberTest_ofApp.h"
#include "TextUtils.h"
#include "BackgroundUpdaterThread.h"

#include "geomConfig.h"

#include "IRDepthTouchTracker.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetFrameRate(60);
	BaseApp::setup();

	setupApps();

	touchTracker = new IRDepthTouchTracker(depthStream, irStream, *bgthread);
	touchTracker->startThread();

	setupDebug();
}

void ofApp::setupApps() {
	/* XXX get rid of hard-coded position constants!! */
	shared_ptr<DesktopWebApp> app1(new DesktopWebApp());
	shared_ptr<DesktopWebApp> app2(new DesktopWebApp());
	app1->origin = ofPoint(-0.924, -0.312, 1.647);
	app1->dx = app2->dx = (ofVec3f(0.084, 0.361, 1.607) - ofPoint(-0.923, 0.366, 1.628)).normalized();
	app1->dy = app2->dy = (ofPoint(-0.923, 0.366, 1.628) - ofVec3f(-0.935, -0.204, 1.644)).normalized();
	app1->width = app2->width = 0.800;
	app1->height = app2->height = 0.600;
	app2->origin = app1->origin + app1->dx*0.900;
	app1->browser.setup(1024, 768);
	app1->browser.loadURL("http://mobile.nytimes.com");
	app2->browser.setup(1024, 768);
	app2->browser.loadURL("http://mudcu.be/sketchpad/");
	app1->buildMesh();
	app2->buildMesh();
	webapps.push_back(app1);
	webapps.push_back(app2);
}

void ofApp::setupDebug() {
	lastDepthTimestamp = 0;
	curDepthFrame = 0;
	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();
	depthviz.allocate(dw, dh, OF_IMAGE_GRAYSCALE);
}

//--------------------------------------------------------------
void ofApp::update(){
	BaseApp::update();

	vector<FingerTouch> newTouches;
	if(touchTracker->update(newTouches)) {
		handleTouches(newTouches);
	}
	
	ofxAwesomium::updateCore();
	for(auto &app : webapps)
		app->browser.update();

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

	/* Fire touch events */
	for(auto &app : webapps) {
		auto touchMapCopy = app->touchMap; // in case the app modifies its touch map during iteration
		for(auto &i : touchMapCopy) {
			int id = i.first;
			if(!allTouches.count(id)) {
				app->onTouchUp(id);
			} else {
				app->onTouchMove(id, getBackgroundWorldPoint(allTouches[id].tip));
			}
		}
	}

	for(auto &i : touchDown) {
		for(auto &app : webapps) {
			if(app->onTouchDown(i, getBackgroundWorldPoint(allTouches[i].tip)))
				break;
		}
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
	auto &depthPixels = irStream.getPixelsRef();
	uint16_t *depthpx = depthPixels.getPixels();
	const int dw = depthPixels.getWidth();
	const int dh = depthPixels.getHeight();

	uint8_t *depthvizpx = depthviz.getPixels();

	/* Convert depth data for visualization purposes */
	for(int i=0; i<dw*dh; i++) {
		depthvizpx[i] = depthpx[i] / 32;
	}

	depthviz.reloadTexture();
}

//--------------------------------------------------------------
void ofApp::drawProjector(){
	/* In this function, draw points in real-world coordinates (metres) */
	for(auto &app : webapps) {
		app->browser.frame.bind();
		ofSetColor(255);
		ofFill();
		app->mesh.draw();
		app->browser.frame.unbind();
	}

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

	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();
	int debugMouseX = mouseX - PROJW;
	int debugMouseY = mouseY;
	if(0 <= debugMouseX && debugMouseX < dw && 0 <= debugMouseY && debugMouseY < dh) {
		ofVec2f pos(debugMouseX, debugMouseY);

		ofPoint bgPt = getBackgroundWorldPoint(pos);
		ofFill();
		ofSetColor(255, 0, 0);
		ofCircle(bgPt, 0.003);
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
	webapps.clear();

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
