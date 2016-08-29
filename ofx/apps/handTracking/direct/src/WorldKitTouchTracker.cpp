//
//  WorldKitTouchTracker.cpp
//  Implementation of WorldKit-style statistical BG subtraction touch tracker.
//
//  Created by Robert Xiao on April 9, 2015.
//
//

#include "WorldKitTouchTracker.h"
#include "TextUtils.h"

vector<FingerTouch> WorldKitTouchTracker::findTouches() {
	vector<FingerTouch> touches;

	buildDiffImage();
	vector<ofVec2f> touchPts = findBlobs();
	for(ofVec2f &pt : touchPts) {
		FingerTouch touch;
		touch.tip.set(pt.x, pt.y);
		touch.touched = true;
		touches.push_back(touch);
	}

	return touches;
}

// InteractorGeometry constants
static const int DIFF_INVALID = 0 << 8; // either background or depth pixel was 0
static const int DIFF_SUBNOISE = 128 << 8; // diff value is below noise threshold
static const int DIFF_THRES_NEAR = 120 << 8; // bg-thres <= depth <= bg-noise
static const int DIFF_THRES_FAR = 135 << 8; // bg+noise <= depth <= bg+thres
static const int DIFF_SENSE_NEAR = 100 << 8; // bg-sensemax <= depth <= bg-thres
static const int DIFF_SENSE_FAR = 150 << 8; // bg+thres <= depth <= bg+sensemax
static const int DIFF_NEAR = 50 << 8; // depth < bg-sensemax
static const int DIFF_FAR = 200 << 8; // depth > bg+sensemax

/** Minimum relative distance to consider above noise */
const static float RELNOISEZ = 6.0f; // z
/** Minimum relative distance to even consider a difference */
const static float RELMINZ = 3.0f; // z
/** Minimum error distance */
const static float SENSEMINZ = 1.0f; // mm
/** Maximum distance to consider a touch */
const static int SENSEMAXZ = 30; // mm

static int constrain(int x, int lo, int hi) {
	if(x < lo) return lo;
	if(x > hi) return hi;
	return x;
}

void WorldKitTouchTracker::buildDiffImage() {
	int n = w*h;
	
	uint16_t *depthPx = depthStream.getPixelsRef().getPixels();
	uint32_t *diffPx = (uint32_t *)diffIm[front].getPixels();

	const float *bgmean = background.getBackgroundMean().getPixels();
	const float *bgstdev = background.getBackgroundStdev().getPixels();

	fill_n(diffPx, n, 0xff000000);

	for(int i=0; i<n; i++) {
		int diffValue = bgmean[i] - depthPx[i];
		if(bgmean[i] == 0 || depthPx[i] == 0) {
			diffPx[i] |= DIFF_INVALID;
			continue;
		}

		float absDiff = abs(diffValue);
		// changed: clamp negative values to avoid halo silliness
		if(diffValue < 0) absDiff = 0;

		float diffRelative = absDiff / (bgstdev[i] + SENSEMINZ);
		diffPx[i] |= constrain(diffRelative/5, 0, 255);
		if(diffRelative < RELMINZ) {
			diffPx[i] += DIFF_SUBNOISE;
		} else if(absDiff > SENSEMAXZ) {
			if (diffValue < 0) {
				diffPx[i] += DIFF_FAR;
			} else {
				diffPx[i] += DIFF_NEAR;
			}
		} else if (diffRelative < RELNOISEZ) {
			/*
			* non-zero, but non-black value: include this in connected
			* components only if the component would also include
			* pixels with high diffs
			*/
			if (diffValue < 0) {
				diffPx[i] += DIFF_THRES_FAR;
			} else {
				diffPx[i] += DIFF_THRES_NEAR;
			}
			diffPx[i] += 2 << 16;
		} else {
			if (diffValue < 0) {
				diffPx[i] += DIFF_SENSE_FAR;
			} else {
				diffPx[i] += DIFF_SENSE_NEAR;
			}
			diffPx[i] += constrain(diffValue * 4, 0, 255) << 16;
		}
	}
}

static const int BLACKTHRESH = 1;
static const int MINBLOBSIZE = 10;
static const int MINPEAKSIZE = 5;
static const int MINPEAKVAL = 3;

static int colorForBlobIndex(int blobId) {
	/* Reverse the bits of the blob ID to make adjacent blob IDs more obvious */

	// https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
	unsigned char b = (unsigned char)blobId;
	b = ((b * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;
	return b;
}

#define GET_ABS(diff) ((diff & 0xff0000) >> 16)

vector<ofVec2f> WorldKitTouchTracker::findBlobs() {
	vector<ofVec2f> blobs;

	int n = w*h;
	
	uint32_t *diffPx = (uint32_t *)diffIm[front].getPixels();
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	fill_n(blobPx, n, 0x00000000);

	for(int idx=0; idx<n; idx++) {
		if(blobPx[idx] != 0 || GET_ABS(diffPx[idx]) < BLACKTHRESH)
			continue;

		vector<unsigned> q;
		int qtail = 0;
		int countPeak = 0;

		q.push_back(idx);

		while(qtail < q.size()) {
			unsigned curidx = q[qtail++];

			blobPx[curidx] |= 0x01;
			if(GET_ABS(diffPx[curidx]) >= MINPEAKVAL)
				countPeak++;

			int y = curidx / w;
			int x = curidx % w;

	#define TEST(dx,dy) do {\
				int xx=x+dx, yy=y+dy; \
				if(0 <= xx && xx < w && 0 <= yy && yy < h) { \
					int otheridx = curidx + dy*w + dx; \
					if(blobPx[otheridx] != 0 || GET_ABS(diffPx[otheridx]) < BLACKTHRESH) \
						continue; \
					q.push_back(otheridx); \
					blobPx[otheridx] |= 0x01; \
				} \
			} while(0)
			// four-way connectivity
			TEST(-1,0);TEST(0,-1);TEST(0,1);TEST(1,0);
	#undef TEST
		}

		if(q.size() < MINBLOBSIZE || countPeak < MINPEAKSIZE) {
			/* Not enough pixels */
			for(auto i : q) {
				blobPx[i] |= 0xff000002; // rejected
			}
			continue;
		}

		/* Enough pixels for the finger */
		ofVec2f pos;
		int count = 0;
		unsigned char label = blobs.size()+1;
		for(auto i : q) {
			blobPx[i] |= 0xff000004 | (label << 16) | (colorForBlobIndex(label) << 8);
			int y = i / w;
			int x = i % w;
			pos += ofVec2f(x, y);
			count++;
		}
		pos /= count;
		blobs.push_back(pos);
	}

	return blobs;
}

vector<FingerTouch> WorldKitTouchTracker::mergeTouches(vector<FingerTouch> &curTouches, vector<FingerTouch> &newTouches) {
    struct touch_dist {
        int cur_index, new_index;
        float dist;
        bool operator<(const struct touch_dist &other) const {
            return dist < other.dist;
        }
    };

    /* Assign each new touch to the nearest cur neighbour */
    for(auto &i : newTouches) {
        i.id = -1;
    }

    vector<touch_dist> distances;
    for(int i=0; i<curTouches.size(); i++) {
        for(int j=0; j<newTouches.size(); j++) {
            float d = curTouches[i].tip.distance(newTouches[j].tip);
            if(d > 100)
                continue;
			touch_dist dist = {i,j,d};
            distances.push_back(dist);
        }
    }
    std::sort(distances.begin(), distances.end());

    for(const auto &i : distances) {
        FingerTouch &curTouch = curTouches[i.cur_index];
        FingerTouch &newTouch = newTouches[i.new_index];
        /* check if already assigned */
        if(curTouch.id < 0 || newTouch.id >= 0)
            continue;
        /* move cur id into new id */
        newTouch.id = curTouch.id;
        curTouch.id = -1;

		/* update other attributes */
        newTouch.touchAge = curTouch.touchAge + 1;
		newTouch.touched = true;
	}

    for(auto &i : newTouches) {
        i.missing = false;
        i.missingAge = 0;
    }

    /* Add 'missing' touches back */
    for(auto &i : curTouches) {
        if(i.id >= 0 && (!i.missing || i.missingAge < 3)) {
            i.missingAge = (i.missing) ? i.missingAge+1 : 0;
            i.missing = true;
            i.statusAge++;
            i.touchAge++;
            newTouches.push_back(i);
        }
    }

    /* Handle new touches and output */
    vector<FingerTouch> finalTouches;
    for(auto &i : newTouches) {
        if(i.id < 0) {
            i.id = nextTouchId++;
            i.statusAge = i.touchAge = 0;
        }
        finalTouches.push_back(i);
    }

    return finalTouches;
}

void WorldKitTouchTracker::threadedFunction() {
	uint64_t lastDepthTimestamp = 0;
	int curDepthFrame = 0;
	fps.fps = 30; // estimated fps

	while(isThreadRunning()) {
		// Check if the depth frame is new
		uint64_t curDepthTimestamp = depthStream.getFrameTimestamp();
		if(lastDepthTimestamp == curDepthTimestamp) {
			ofSleepMillis(5);
			continue;
		}
		lastDepthTimestamp = curDepthTimestamp;
		curDepthFrame++;
		fps.update();
		
		vector<FingerTouch> newTouches = findTouches();
		vector<FingerTouch> curTouches = touches;
		{
			ofScopedLock lock(touchLock);
			touches = mergeTouches(curTouches, newTouches);
			touchesUpdated = true;
		}

		front = !front;
	}
}

void WorldKitTouchTracker::drawDebug(float x, float y) {
	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();

	int back = !front;

	diffIm[back].reloadTexture();
	blobIm[back].reloadTexture();

	diffIm[back].draw(x, y);
	blobIm[back].draw(x, y+dh);
	
	drawText("Diff", x, y, HAlign::left, VAlign::top);
	drawText("Blob", x, y+dh, HAlign::left, VAlign::top);
}

/* update() function called from the main thread */
bool WorldKitTouchTracker::update(vector<FingerTouch> &retTouches) {
	fps.tick();

	ofScopedLock lock(touchLock);
	if(touchesUpdated) {
		retTouches = touches;
		touchesUpdated = false;
		return true;
	} else {
		return false;
	}
}

WorldKitTouchTracker::~WorldKitTouchTracker() {
	stopThread();
	waitForThread();
}

WorldKitTouchTracker::WorldKitTouchTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background)
: TouchTracker(depthStream, irStream, background) {
	front = 0;

	for(int i=0; i<2; i++) {
		diffIm[i].allocate(w, h, OF_IMAGE_COLOR_ALPHA);
		blobIm[i].allocate(w, h, OF_IMAGE_COLOR_ALPHA);
	}
}
