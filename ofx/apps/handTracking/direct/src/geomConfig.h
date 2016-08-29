//
//  geomConfig.h
//  Calibration and screen settings shared by all apps
//
//  Created by Robert Xiao on April 9, 2015.
//
//

#pragma once

#include "ofMath.h"

/* Projector display is assumed to be to the left of the main display */
#define PROJW 1920
#define PROJH 1080

#define DISPW 1920
#define DISPH 1080

/* Matrix calibration from calibrate.py */
const static ofMatrix4x4 projector_projmat(
		56.840041f,-0.120907f,22.803011f,19.316478f,
		0.561777f,58.463123f,23.164536f,-6.195360f,
		-0.000581f,0.001136f,0.027288f,-0.029459f,
		-0.000475f,0.000929f,0.022327f,0.016536f);
const static ofMatrix4x4 projector_transpose = ofMatrix4x4::getTransposedOf(projector_projmat);
