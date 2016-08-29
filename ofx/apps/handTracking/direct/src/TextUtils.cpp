//
//  TextUtils.cpp
//  Utilities for displaying positioned text.
//
//  Created by Robert Xiao on May 28, 2014.
//
//

#include "TextUtils.h"

#define BITMAP_CHAR_WIDTH 8
#define BITMAP_CHAR_LINE_HEIGHT 13
#define BITMAP_CHAR_ASCENDER 10

static HAlign horiz_current = HAlign::left;
static VAlign vert_current = VAlign::top;

/* http://stackoverflow.com/a/236803 */
static vector<string> split(const string &s, char delim) {
    vector<string> elems;
    stringstream ss(s);
    string item;
    while(getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

void setTextAlign(HAlign halign, VAlign valign) {
    if(halign != HAlign::current)
        horiz_current = halign;
    if(valign != VAlign::current)
        vert_current = valign;
}

static void _drawBitmapText(const string &text, float x, float y, float w, float h, HAlign halign, VAlign valign) {
    if(halign == HAlign::current)
        halign = horiz_current;
    if(valign == VAlign::current)
        valign = vert_current;

    vector<string> bits = split(text, '\n');
    float textheight = BITMAP_CHAR_LINE_HEIGHT * bits.size();
    float cury;
    switch(valign) {
        default:
        case VAlign::top:
            cury = y;
            break;
        case VAlign::center:
            cury = y + (h-textheight)/2;
            break;
        case VAlign::bottom:
            cury = y + h - textheight;
            break;
    }
    // adjust for ofDrawBitmapString drawing from the baseline
    cury += BITMAP_CHAR_ASCENDER;

    for(const auto &s : bits) {
        float textwidth = BITMAP_CHAR_WIDTH * s.length();
        float curx;
        switch(halign) {
            default:
            case HAlign::left:
                curx = x;
                break;
            case HAlign::center:
                curx = x + (w - textwidth)/2;
                break;
            case HAlign::right:
                curx = x + w - textwidth;
                break;
        }
        ofDrawBitmapString(s, curx, cury);
        cury += BITMAP_CHAR_LINE_HEIGHT;
    }
}

static void _drawFontText(ofTrueTypeFont &font, const string &text, float x, float y, float w, float h, HAlign halign, VAlign valign) {
    if(halign == HAlign::current)
        halign = horiz_current;
    if(valign == VAlign::current)
        valign = vert_current;
    
    vector<string> bits = split(text, '\n');
    float lineheight = font.getLineHeight();
    float textheight = lineheight * bits.size();
    float cury;
    switch(valign) {
        default:
        case VAlign::top:
            cury = y;
            break;
        case VAlign::center:
            cury = y + (h - textheight)/2;
            break;
        case VAlign::bottom:
            cury = y + h - textheight;
            break;
    }
	cury += font.getLineHeight(); //font.getAscenderHeight(); // drawString draws from the text baseline

    for(const auto &s : bits) {
        ofRectangle bbox = font.getStringBoundingBox(s, 0, 0);
        float textwidth = bbox.x + bbox.width + 1;
        float curx;
        switch(halign) {
            default:
            case HAlign::left:
                curx = x;
                break;
            case HAlign::center:
                curx = x + (w - textwidth)/2;
                break;
            case HAlign::right:
                curx = x + w - textwidth;
                break;
        }
        font.drawString(s, curx, cury);
        cury += lineheight;
    }
}

void drawText(const string &text, float x, float y, HAlign halign, VAlign valign) {
    _drawBitmapText(text, x, y, 0, 0, halign, valign);
}

void drawText(const string &text, const ofPoint &pt, HAlign halign, VAlign valign) {
    _drawBitmapText(text, pt.x, pt.y, 0, 0, halign, valign);
}

void drawText(const string &text, const ofRectangle &rect, HAlign halign, VAlign valign) {
    _drawBitmapText(text, rect.x, rect.y, rect.width, rect.height, halign, valign);
}

void drawText(const string &text, float x, float y, float w, float h, HAlign halign, VAlign valign) {
    _drawBitmapText(text, x, y, w, h, halign, valign);
}

void drawText(ofTrueTypeFont &font, const string &text, float x, float y, HAlign halign, VAlign valign) {
    _drawFontText(font, text, x, y, 0, 0, halign, valign);
}

void drawText(ofTrueTypeFont &font, const string &text, const ofPoint &pt, HAlign halign, VAlign valign) {
    _drawFontText(font, text, pt.x, pt.y, 0, 0, halign, valign);
}

void drawText(ofTrueTypeFont &font, const string &text, const ofRectangle &rect, HAlign halign, VAlign valign) {
    _drawFontText(font, text, rect.x, rect.y, rect.width, rect.height, halign, valign);
}

void drawText(ofTrueTypeFont &font, const string &text, float x, float y, float w, float h, HAlign halign, VAlign valign) {
    _drawFontText(font, text, x, y, w, h, halign, valign);
}
