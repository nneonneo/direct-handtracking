//
//  WilsonTouchTracker.cpp
//  Abstract implementation of Andy Wilson's background-subtraction touch tracker.
// 
//  Created by Robert Xiao on April 9, 2015.
//
//

#include "WilsonTouchTracker.h"
#include "TextUtils.h"

void WilsonTouchTracker::doDepthThresh(const uint16_t *bgPx, int tlow, int thigh) {
	int n = w*h;
	
	uint16_t *depthPx = depthStream.getPixelsRef().getPixels();
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	for(int i=0; i<n; i++) {
		int diff = bgPx[i] - depthPx[i];
		if(diff >= tlow && diff <= thigh) {
			blobPx[i] = 0xffffff00;
		} else {
			blobPx[i] = 0xff000000;
		}
	}
}

void WilsonTouchTracker::boxcarFilterH(unsigned char *pixels, int w, int h, int channel, int filtersz) {
	vector<int> rowsum(w);

	for(int y=0; y<h; y++) {
		unsigned char *row = pixels + (4*y*w);
		fill(rowsum.begin(), rowsum.end(), 0);
		for(int x=1; x<w; x++) {
			rowsum[x] = rowsum[x-1] + row[x*4 + channel];
		}

		int x=0;
		for(; x<filtersz+1; x++) {
			row[x*4+channel] = 0;
		}
		int div = filtersz * 2 + 1;
		for(; x<w-filtersz; x++) {
			row[x*4+channel] = (rowsum[x+filtersz] - rowsum[x-filtersz-1]) / div;
		}
		for(; x<w; x++) {
			row[x*4+channel] = 0;
		}
	}
}

void WilsonTouchTracker::boxcarFilterV(unsigned char *pixels, int w, int h, int channel, int filtersz) {
	vector<int> colsum(h);

	for(int x=0; x<w; x++) {
		unsigned char *col = pixels + (4*x);
		fill(colsum.begin(), colsum.end(), 0);
		for(int y=1; y<h; y++) {
			colsum[y] = colsum[y-1] + col[4*y*w + channel];
		}

		int y=0;
		for(; y<filtersz+1; y++) {
			col[4*y*w + channel] = 0;
		}
		int div = filtersz * 2 + 1;
		for(; y<h-filtersz; y++) {
			col[4*y*w + channel] = (colsum[y+filtersz] - colsum[y-filtersz-1]) / div;
		}
		for(; y<h; y++) {
			col[4*y*w + channel] = 0;
		}
	}
}

void WilsonTouchTracker::doLowpassFilter(int filtersz, int thresh) {
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	boxcarFilterH((uint8_t *)blobPx, w, h, 1, filtersz);
	boxcarFilterV((uint8_t *)blobPx, w, h, 1, filtersz);

	int n = w*h;

	for(int i=0; i<n; i++) {
		if(((blobPx[i] & 0xff00) >> 8) > thresh)
			blobPx[i] |= 0xff0000ff;
	}
}

vector<ofVec2f> WilsonTouchTracker::findBlobs(int minsize) {
	vector<ofVec2f> blobs;
	
	int n = w*h;
	
	uint16_t *depthPx = depthStream.getPixelsRef().getPixels();
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	for(int idx=0; idx<n; idx++) {
		// must be filter-selected and not visited
		if((blobPx[idx] & 0xff) != 0x00ff)
			continue;

		vector<unsigned> q, q2;
		int qtail = 0;

		q.push_back(idx);

		while(qtail < q.size()) {
			unsigned curidx = q[qtail++];

			blobPx[curidx] &= ~0xfe;

			int y = curidx / w;
			int x = curidx % w;

	#define TEST(dx,dy) do {\
				int xx=x+dx, yy=y+dy; \
				if(0 <= xx && xx < w && 0 <= yy && yy < h) { \
					int otheridx = curidx + dy*w + dx; \
					if((blobPx[otheridx] & 0xff) != 0xff) \
						continue; \
					q.push_back(otheridx); \
					blobPx[otheridx] &= ~0xfe; \
				} \
			} while(0)
			// four-way connectivity
			TEST(-1,0);TEST(0,-1);TEST(0,1);TEST(1,0);
	#undef TEST
		}

		if(q.size() < minsize) {
			/* Not enough pixels */
			continue;
		}

		/* Enough pixels for the finger */
		ofVec2f pos;
		int count = 0;
		for(auto i : q) {
			blobPx[i] |= 0x80;
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

vector<FingerTouch> WilsonTouchTracker::mergeTouches(vector<FingerTouch> &curTouches, vector<FingerTouch> &newTouches) {
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

void WilsonTouchTracker::threadedFunction() {
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

void WilsonTouchTracker::drawDebug(float x, float y) {
	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();

	int back = !front;

	blobIm[back].reloadTexture();

	blobIm[back].draw(x, y);

	drawText("Blob", x, y, HAlign::left, VAlign::top);
}

/* update() function called from the main thread */
bool WilsonTouchTracker::update(vector<FingerTouch> &retTouches) {
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

WilsonTouchTracker::WilsonTouchTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background)
: TouchTracker(depthStream, irStream, background) {
	front = 0;

	for(int i=0; i<2; i++) {
		blobIm[i].allocate(w, h, OF_IMAGE_COLOR_ALPHA);
	}
}
