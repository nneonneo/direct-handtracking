//
//  WindowUtils.cpp
//  Utilities for manipulating the openFrameworks window. Windows-only for now.
//
//  Created by Robert Xiao on Jan 30, 2015.
//
//

#include "WindowUtils.h"
#include "ofMain.h"
#include <Windows.h>

void removeWindowBorder() {
	HWND hwnd = ofGetWin32Window();
	if(!hwnd) {
		ofLogError() << "Failed to find Win32 window for active app";
		return;
	}

	long windowStyle = GetWindowLong(hwnd, GWL_STYLE);
	windowStyle &= ~WS_OVERLAPPEDWINDOW;
	windowStyle |= WS_POPUP;
	SetWindowLong(hwnd, GWL_STYLE, windowStyle);
}
