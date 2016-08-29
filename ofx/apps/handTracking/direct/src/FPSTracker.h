//
//  FPSTracker.h
//  Simple FPS tracker.
//
//  Created by Robert Xiao on Apr 12, 2014.
//
//

#pragma once

#include "ofMain.h"

class FPSTracker {
    ofMutex mutex;
    
public:
    double fps;
    double lastTime;
    
    FPSTracker() : fps(0.0), lastTime(0.0) {}
    
    void reset() {
        ofScopedLock lock(mutex);
        
        fps = 0.0;
        lastTime = ofGetElapsedTimeMillis();
    }

    /* Tick the FPS tracker.
     Call with a fixed period (e.g. every 1/60th of a second) */
    void tick(double alpha=0.10) {
        ofScopedLock lock(mutex);
        
        double now = ofGetElapsedTimeMillis();
        if(now - lastTime < 0.00001)
            return;
        
        double newfps = 1000.0 / (now - lastTime);
        if(newfps < fps)
            fps += alpha * (newfps - fps);
    }
    
    /* Update the FPS tracker.
     Call when a new frame arrives. */
    void update(double alpha=0.10) {
        ofScopedLock lock(mutex);
        
        double now = ofGetElapsedTimeMillis();
        if(now - lastTime < 0.00001)
            return;
        
        double newfps = 1000.0 / (now - lastTime);
        fps += alpha * (newfps - fps);
        lastTime = now;
    }
};
