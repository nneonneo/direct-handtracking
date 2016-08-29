//
//  StudyTask.cpp
//  Base class for study tasks.
// 
//  Created by Robert Xiao on April 10, 2015.
//
//

#include "StudyTask.h"
#include <ctime>
#include <io.h> // _chsize

string StudyTask::GetTimestampString() {
	return ofGetTimestampString("%Y%m%d-%H%M%S");
}

double StudyTask::GetTimestamp() {
	/* cribbed from the gettimeofday function for Windows */
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    FILETIME fileTime;
	SYSTEMTIME systemTime;

	GetSystemTime(&systemTime);
	SystemTimeToFileTime(&systemTime, &fileTime);

	uint64_t time = ((uint64_t)fileTime.dwLowDateTime)
				  | (((uint64_t)fileTime.dwHighDateTime) << 32);

	return ((time - EPOCH) / 10000000L) + systemTime.wMilliseconds / 1000.0f;
}

StudyTask::StudyTask(const string &session, const string &tag, const vector<StudyTouchTracker> &trackers, BaseApp &app)
	: session(session), tag(tag), trackers(trackers), app(app) {

	ofDirectory::createDirectory("record", true, true);
	string path = ofToDataPath("record/" + session + "-"  + tag + ".csv");
	recf = fopen(path.c_str(), "w+");
	if(!recf) {
		throw std::runtime_error("can't open task log file");
	}
	currentTrial = 0;
}

void StudyTask::recordHeader(const string &header) {
	fprintf(recf, "%s\n", header.c_str());
	fflush(recf);
}

void StudyTask::recordTrial(const string &row) {
	trialRows.push_back(ftell(recf));
	fprintf(recf, "%s\n", row.c_str());
	fflush(recf);
	currentTrial++;
}

bool StudyTask::unrecordTrial() {
	if(currentTrial == 0)
		return false;
	currentTrial--;
	long pos = trialRows.back();
	trialRows.pop_back();
	fseek(recf, pos, SEEK_SET);
	_chsize(fileno(recf), pos); // msvc-specific
	return true;
}
