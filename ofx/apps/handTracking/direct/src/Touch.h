/* Touch point classes. */

#pragma once
#include "ofMain.h"

/// Filtered touch point
struct FingerTouch {
    /// ID of the touch
    int id;

    /// Coordinates of the fingertip
    ofPoint tip;
	
    /// Coordinates of the finger base (approximate)
    ofPoint base;

    /// Is this touch currently contacting the surface?
    bool touched;

    /// Number of frames since the touch state changed
    int statusAge;

    /// Number of frames since the touch first appeared
    int touchAge;

    FingerTouch() : id(-1), tip(), touched(false),
    statusAge(0), touchAge(0), touchZ(0), missing(false), missingAge(0) {

    }
public:
	/* n.b. these should not be accessed by classes besides the touch trackers */
	/// Touch Z height
	float touchZ;

    /// Did this touch recently go missing?
    bool missing;

    /// Number of frames since the touch went missing.
    int missingAge;
};
