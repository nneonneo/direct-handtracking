//
//  WebApp.cpp
//  App wrapper to allow web apps to appear on the desktop surface.
// 
//  Created by Robert Xiao on April 8, 2015.
//
//

#pragma once

#include "ofMain.h"
#include "ofxAwesomium.h"
#include "Touch.h"

class DesktopWebApp {
public:
	ofMesh mesh;

	ofVec3f dx, dy; // world dx, dy
	ofPoint origin; // world top-left
	float width, height; // metres
    map<int, ofVec2f> touchMap;
	int lmbId;

	ofxAwesomium browser;

	DesktopWebApp() : lmbId(-1) {
	}

	void buildMesh() {
		// call me after setting up all public fields
		int bw = browser.getWidth();
		int bh = browser.getHeight();
		mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
		mesh.addVertex(origin);
		mesh.addTexCoord(ofVec2f(0, 0));
		mesh.addVertex(origin+dx*width);
		mesh.addTexCoord(ofVec2f(bw, 0));
		mesh.addVertex(origin+dy*height);
		mesh.addTexCoord(ofVec2f(0, bh));
		mesh.addVertex(origin+dx*width+dy*height);
		mesh.addTexCoord(ofVec2f(bw, bh));
	}

    ofVec2f worldToBrowser(const ofVec3f &worldPt) {
        return ofVec2f(
            (worldPt - origin).dot(dx) / width * browser.getWidth(),
            (worldPt - origin).dot(dy) / height * browser.getHeight()
        );
    }

    bool onTouchDown(int id, const ofVec3f &worldPt) {
        ofVec2f pt = worldToBrowser(worldPt);
        if(pt.x < 0 || pt.x >= browser.getWidth() || pt.y < 0 || pt.y >= browser.getHeight()) {
            return false;
        }

		if(lmbId == -1) {
			lmbId = id;
			browser.mouseMoved(pt.x, pt.y);
			browser.mousePressed(pt.x, pt.y, 0);
		}
        touchMap[id] = pt;
        return true;
    }

    bool onTouchMove(int id, const ofVec3f &worldPt) {
        ofVec2f pt = worldToBrowser(worldPt);
		if(id == lmbId)
	        browser.mouseDragged(pt.x, pt.y, 0);
        touchMap[id] = pt;
        return true;
    }

    bool onTouchUp(int id) {
        ofVec2f pt = touchMap[id];
        //browser.mouseMoved(pt.x, pt.y);
		if(id == lmbId) {
	        browser.mouseReleased(pt.x, pt.y, 0);
			lmbId = -1;
		}
		touchMap.erase(id);
        return true;
    }
};
