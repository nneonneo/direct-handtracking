//
//  TextUtils.h
//  Utilities for displaying positioned text.
//
//  Created by Robert Xiao on May 28, 2014.
//
//

#pragma once

#include "ofMain.h"

enum class HAlign {
    current=-1,
    left,
    center,
    right,
};

enum class VAlign {
    current=-1,
    top,
    center,
    bottom,
};

void setTextAlign(HAlign halign=HAlign::current, VAlign valign=VAlign::current);
/* Draw text at a point. The text is anchored according to the alignment setting
 (e.g. 'right,bottom' will treat the point as the bottom-right corner of the text). */
void drawText(const string &text, float x, float y, HAlign halign=HAlign::current, VAlign valign=VAlign::current);
void drawText(const string &text, const ofPoint &pt, HAlign halign=HAlign::current, VAlign valign=VAlign::current);
/* Draw text inside a rectangle. The text will be drawn in the position specified by the alignment setting
 (e.g. 'center,bottom' will draw the text horizontally centered at the bottom of the rectangle). */
void drawText(const string &text, float x, float y, float w, float h, HAlign halign=HAlign::current, VAlign valign=VAlign::current);
void drawText(const string &text, const ofRectangle &rect, HAlign halign=HAlign::current, VAlign valign=VAlign::current);

/* Draw text at a point. The text is anchored according to the alignment setting
 (e.g. 'right,bottom' will treat the point as the bottom-right corner of the text). */
void drawText(ofTrueTypeFont &font, const string &text, float x, float y, HAlign halign=HAlign::current, VAlign valign=VAlign::current);
void drawText(ofTrueTypeFont &font, const string &text, const ofPoint &pt, HAlign halign=HAlign::current, VAlign valign=VAlign::current);
/* Draw text inside a rectangle. The text will be drawn in the position specified by the alignment setting
 (e.g. 'center,bottom' will draw the text horizontally centered at the bottom of the rectangle). */
void drawText(ofTrueTypeFont &font, const string &text, float x, float y, float w, float h, HAlign halign=HAlign::current, VAlign valign=VAlign::current);
void drawText(ofTrueTypeFont &font, const string &text, const ofRectangle &rect, HAlign halign=HAlign::current, VAlign valign=VAlign::current);
