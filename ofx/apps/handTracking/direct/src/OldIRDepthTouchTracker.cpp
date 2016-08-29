#include "OldIRDepthTouchTracker.h"

// constants
const float DEPTH_NOISE_Z = 1.0f; // z values below this threshold are considered pure noise
const uint16_t DEPTH_MAX_DIFF = 100; // distances farther than this above the background are considered simply too far away

#pragma region Touch Detection
static struct BlobColors {
	static const int MAXCOLORS = 256;
	uint32_t colors[MAXCOLORS];
	static uint32_t color(int r, int g, int b) {
		return (b << 16) | (g << 8) | r;
	}
	BlobColors() {
		colors[0] = color(0, 0, 0);
		colors[1] = color(255, 255, 1);
		colors[2] = color(150, 255, 2);
		colors[3] = color(200, 100, 3);
		colors[4] = color(150, 50, 4);
		colors[5] = color(100, 50, 5);
		colors[6] = color(255, 100, 6);
		colors[7] = color(100, 50, 7);
		colors[8] = color(100, 100, 8);
		colors[9] = color(150, 150, 9);
		colors[10] = color(50, 100, 10);
		colors[11] = color(204, 77, 11);
		colors[12] = color(51, 204, 12);
		colors[13] = color(102, 204, 13);
		colors[14] = color(179, 204, 14);
		colors[15] = color(204, 153, 15);

		for (int k=15;k<MAXCOLORS;k++)
			colors[k] = color((int)ofRandom(100), (int)ofRandom(100), k);
	}
	const uint32_t &operator[](int index) const {
		return colors[index];
	}
} colors;

void OldIRDepthTouchTracker::fillIrCannyHoles() {
	const int n = w * h;
	/* 255 = insignificant canny
	   224 = unvisited significant canny
	   208 = seen, unvisited significant canny
	   192 = visited significant canny
	   160 = fill candidate
	   128 = filled significant canny
	   0 = no canny */
	uint8_t *ircannypx = irCanny.getPixels();
	uint32_t *diffpx = (uint32_t *)diffimage.getPixels();

	/* Mark pixels near "flat" areas as significant */
	const int FLATWIN = 5; // flatness check window size
	for(int i=w*FLATWIN; i<n-w*FLATWIN; i++) {
		if(ircannypx[i] != 255)
			continue;

		/* Check that nearby pixels are also near the background.
		This check eliminates gradiated pixels on the edges of arms, knuckles, etc. */
		for(int dy=-1; dy<=1; dy++) {
			for(int dx=-1; dx<=1; dx++) {
				int diffval = diffpx[i+dx*FLATWIN+dy*FLATWIN*w];
				// Reject if the difference (z-value) is too big, or if the pixel is invalid.
				if((diffval & 0xffff) > 30 || (diffval & 0xff000000) == 0) {
					goto not_flat;
				}
			}
		}
		ircannypx[i] = 224;
		continue;
not_flat:
		;
	}

	/* Find significant pixels and fill outwards */
	int *queue = new int[w*h]; // queue for above-threshold pixels

	for(int sy=0; sy<h; sy++) {
		for(int sx=0; sx<w; sx++) {
			int idx = sy*w + sx;
			if(ircannypx[idx] != 224)
				continue;

			int queuehead = 0, queuetail = 0;
			queue[queuetail++] = idx;
			ircannypx[idx] = 208;
			while(queuehead < queuetail) {
				int curidx = queue[queuehead++];
				int curpx = ircannypx[curidx];

				if(curpx == 208) {
					ircannypx[curidx] = 192;
				}

				int y = curidx / w;
				int x = curidx % w;
				int found = 0;

				// Find unvisited significant neighbours
#define TEST(dx,dy) do { int xx = x+dx, yy = y+dy; \
				if(0 <= xx && xx < w && 0 <= yy && yy < h) { \
					int otheridx = curidx + dy*w + dx; \
					if(ircannypx[otheridx] <= 192) \
						continue; /* must be unvisited, real canny pixels */ \
					found++; \
					if(ircannypx[otheridx] != 224) \
						continue; /* already visited, or not significant */ \
					queue[queuetail++] = otheridx; \
					ircannypx[otheridx] = 208; \
				} \
				} while(0)
				/* Eight-way neighbours, to cross diagonals */
				TEST(-1, -1);
				TEST(-1, 0);
				TEST(-1, 1);
				TEST(0, -1);
				TEST(0, 1);
				TEST(1, -1);
				TEST(1, 0);
				TEST(1, 1);
#undef TEST

				if(curpx == 160) {
					/* fill candidate */
					if(found) {
						ircannypx[curidx] = 128;
					} else {
						ircannypx[curidx] = 0;
					}
				} else if(!found) {
					/* Mark all neighbours as fill candidates */
#define TEST(dx,dy) do { int xx = x+dx, yy = y+dy; \
				if(0 <= xx && xx < w && 0 <= yy && yy < h) { \
					int otheridx = curidx + dy*w + dx; \
					if(ircannypx[otheridx] != 0) \
						continue; /* must be blank pixel */ \
					queue[queuetail++] = otheridx; \
					ircannypx[otheridx] = 160; \
				} \
				} while(0)
				/* Eight-way neighbours, to cross diagonals */
				TEST(-1, -1);
				TEST(-1, 0);
				TEST(-1, 1);
				TEST(0, -1);
				TEST(0, 1);
				TEST(1, -1);
				TEST(1, 0);
				TEST(1, 1);
#undef TEST
				}
			}
		}
	}

	delete[] queue;
}


/* Segment difference image into connected components */
void OldIRDepthTouchTracker::touchDetectionConnectedComponents(const uint32_t *src, const uint8_t *edges, uint32_t *labels) {
	static const int BLACKTHRESH = 1;
	static const int MINPEAKSIZE = 300; // hand+arm size (approx)
	static const int MINPEAKVAL = 25;
	static const int MAXBOUNDARYDIST = 45;
	static const int MAXDIFF = 20; // mm

	memset(labels, 0, w*h*sizeof(uint32_t));

	int *queue = new int[w*h]; // queue for above-threshold pixels
	int *queue2 = new int[w*h]; // queue for below-threshold pixels (scanned up to a certain distance from the nearest above-threshold pixel)
	int curlabel = 1;

	/* Color components: A=255-dist B=blobidx G,R=coloring */
	for(int sy=0; sy<h; sy++) {
		for(int sx=0; sx<w; sx++) {
			int idx = sy*w + sx;
			int px = src[idx] & 0xffff;
			if(labels[idx] != 0 || px < MINPEAKVAL)
				continue;

			// Scan above-threshold pixels
			int count = 0, countPeak = 0;
			int queuehead = 0, queuetail = 0;
			int queue2head = 0, queue2tail = 0;
			queue[queuetail++] = idx;
			labels[idx] = colors[curlabel] | ((0xff - 0) << 24);
			while(queuehead < queuetail) {
				int curidx = queue[queuehead++];
				int curpx = src[curidx] & 0xffff;
				count++;
				countPeak++;

				int y = curidx / w;
				int x = curidx % w;
#define TEST(dx,dy) do { int xx = x+dx, yy = y+dy; \
				if(0 <= xx && xx < w && 0 <= yy && yy < h) { \
					int otheridx = curidx + dy*w + dx; \
					int otherpx = src[otheridx] & 0xffff; \
					if(labels[otheridx] != 0 || otherpx < BLACKTHRESH || abs(otherpx - curpx) > MAXDIFF) \
						continue; \
					if(otherpx < MINPEAKVAL) { \
						/* Boundary value */ \
						queue2[queue2tail++] = otheridx | (1 << 24); \
					} else { \
						queue[queuetail++] = otheridx; \
						labels[otheridx] = colors[curlabel] | ((0xff - 0) << 24); \
					} \
				} \
				} while(0)
				TEST(-1, 0);
				TEST(1, 0);
				TEST(0, -1);
				TEST(0, 1);
#undef TEST
			}

			if(countPeak < MINPEAKSIZE) {
				/* Reject blob */
				for(int i=0; i<queuehead; i++) {
					/* Mark as visited, but not part of any blob */
					labels[queue[i]] = 0x01000000;
				}
				continue;
			}

			// Scan below-threshold pixels
			while(queue2head < queue2tail) {
				int curidx = queue2[queue2head++];
				int dist = curidx >> 24;
				curidx = curidx & 0xffffff;
				int curpx = src[curidx] & 0xff;

				if(labels[curidx] != 0) {
					continue;
				}
				if(edges[curidx] != 0) {
					// hit an edge
					continue;
				}
				labels[curidx] = colors[curlabel] | ((0x80 - dist) << 24);
				count++;
				if(dist >= MAXBOUNDARYDIST)
					continue;
				dist++;

				int y = curidx / w;
				int x = curidx % w;
#define TEST(dx,dy) do { int xx = x+dx, yy = y+dy; \
				if(0 <= xx && xx < w && 0 <= yy && yy < h) { \
					int otheridx = curidx + dy*w + dx; \
					int otherpx = src[otheridx] & 0xff; \
					if(labels[otheridx] != 0) continue; \
					queue2[queue2tail++] = otheridx | (dist << 24); \
				} \
				} while(0)
				TEST(-1, 0);
				TEST(1, 0);
				TEST(0, -1);
				TEST(0, 1);
#undef TEST
			}

			curlabel++;
		}
	}

	delete[] queue;
	delete[] queue2;
}
#pragma endregion

#pragma region Touch Tracking
vector<FingerTouch> OldIRDepthTouchTracker::touchTrackingConnectedComponents(uint32_t *touchpx) {
	static const int MIN_BLOB_SIZE = 4;

	int *queue = new int[w*h];
	int curlabel = 1;
	vector<FingerTouch> touches;

	/* Color components: A=255 B=blobidx G=diff R=distance */
	for(int sy=0; sy<h; sy++) {
		for(int sx=0; sx<w; sx++) {
			int idx = sy*w + sx;
			if((touchpx[idx] >> 16) != 0xff00)
				continue; // pixel already labeled (B != 0), or not valid (A != 0xff)

			int count = 0;
			ofVec2f furthestPos;
			int furthestDist = 0;
			int sumDiff = 0;

			int queuehead = 0, queuetail = 0;
			queue[queuetail++] = idx;
			touchpx[idx] |= (curlabel << 16);
			while(queuehead < queuetail) {
				int curidx = queue[queuehead++];
				int curpx = touchpx[curidx];
				count++;
				sumDiff += ((curpx >> 8) & 0xff) - 0x80;

				int y = curidx / w;
				int x = curidx % w;

				int curdist = curpx & 0xff;
				if(curdist > furthestDist) {
					furthestPos.set(x, y);
					furthestDist = curdist;
				}

#define TEST(dx,dy) do { int xx = x+dx, yy = y+dy; \
				if(0 <= xx && xx < w && 0 <= yy && yy < h) { \
					int otheridx = curidx + dy*w + dx; \
					if((touchpx[otheridx] >> 16) != 0xff00) \
						continue; \
					queue[queuetail++] = otheridx; \
					touchpx[otheridx] |= (curlabel << 16); \
				} \
				} while(0)
				/* Eight-way neighbours, to cross diagonals */
				TEST(-1, -1);
				TEST(-1, 0);
				TEST(-1, 1);
				TEST(0, -1);
				TEST(0, 1);
				TEST(1, -1);
				TEST(1, 0);
				TEST(1, 1);
#undef TEST
			}

			if(count < MIN_BLOB_SIZE) {
				/* Reject blob */
				for(int i=0; i<queuehead; i++) {
					/* Mark as visited, but not part of any blob */
					touchpx[queue[i]] = 0x4000ff00;
				}
				continue;
			}

			/* Valid blob! */
			FingerTouch tp;
			tp.touched = true;//(sumDiff / (float)count) < 6;
			tp.id = curlabel;
			tp.tip = furthestPos;
			touches.push_back(tp);

			curlabel++;
			if(curlabel == 256)
				curlabel = 1; // avoid 0
		}
	}

	delete[] queue;
	return touches;
}

vector<FingerTouch> OldIRDepthTouchTracker::mergeTouches(vector<FingerTouch> &curTouches, vector<FingerTouch> &newTouches) {
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
        /* copy other attributes */
        if(curTouch.touched == newTouch.touched) {
            newTouch.statusAge = curTouch.statusAge + 1;
        } else {
            newTouch.statusAge = 0;
        }
        newTouch.touchAge = curTouch.touchAge + 1;
		/* EWMA new touch */
		newTouch.tip = 0.50 * (newTouch.tip - curTouch.tip) + curTouch.tip;
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
#pragma endregion

void OldIRDepthTouchTracker::threadedFunction() {
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

		/* Setup images for touch tracking */
		auto &depthPixels = depthStream.getPixelsRef();
		auto &irPixels = irStream.getPixelsRef();
		uint16_t *depthpx = depthPixels.getPixels();
		uint16_t *irpx = irPixels.getPixels();
		const int n = w * h;

		uint32_t *diffpx = (uint32_t *)diffimage.getPixels();
		uint32_t *blobpx = (uint32_t *)blobviz.getPixels();
		uint32_t *touchpx = (uint32_t *)touchviz.getPixels();
		uint8_t *ircannypx = irCanny.getPixels();
		const float *bgmean = background.getBackgroundMean().getPixels();
		const float *bgstdev = background.getBackgroundStdev().getPixels();

		/* Canny the IR image */
		for(int i=0; i<n; i++) {
			ircannypx[i] = irpx[i] / 64;
		}
		cv::Mat irCannyMat(irCanny.getCvImage());
		/* Edge finding, lightly tuned parameters */
		cv::Canny(irCannyMat, irCannyMat, 4000, 8000, 7, true);

		/* Update diff image */
		for(int i=0; i<n; i++) {
			/* Update diff image */
			float diff;
			float z;
			if(depthpx[i]) {
				diff = bgmean[i] - depthpx[i];
				z = diff / bgstdev[i];
			} else {
				diff = 0;
				z = 0;
			}
			// pixels stored in ABGR order: A=valid, B=zone (127=normal, 255=too far), GR=z-value
			if(bgmean[i] == 0) diffpx[i] = 0;
			else if(z < DEPTH_NOISE_Z) diffpx[i] = 0xff000000;
			else if(diff < DEPTH_MAX_DIFF) diffpx[i] = 0xff7f0000 | (uint16_t)diff;
			else diffpx[i] = 0xffff0000 | (uint16_t)diff;
		}
		
		fillIrCannyHoles();

		/* Find blobs in the diff image */
		touchDetectionConnectedComponents(diffpx, ircannypx, blobpx);

		/* Loop over blobs and label them */
		memset(touchpx, 0, n*sizeof(uint32_t));
		const int FLATWIN = 5; // flatness check window size
		for(int i=w*FLATWIN; i<n-w*FLATWIN; i++) {
			int label = blobpx[i] & 0xff;
			int dist = blobpx[i] >> 24;
			if(dist == 0)
				continue;
			blobpx[i] = 0x4000ff00;
			/* Check that the pixel is a below-threshold pixel,
			and that it is at least 2 pixels away from an above-threshold pixel.
			This helps to eliminate noisy gradiated pixels surrounding sharp transitions in the depth image. */
			if(0 < dist && dist < 0x80 && label != 0) {
				/* Check that nearby pixels are also near the background.
				This check eliminates gradiated pixels on the edges of arms, knuckles, etc. */
				for(int dy=-1; dy<=1; dy++) {
					for(int dx=-1; dx<=1; dx++) {
						int diffval = diffpx[i+dx*FLATWIN+dy*FLATWIN*w];
						// Reject if the difference (z-value) is too big, or if the pixel is invalid.
						if((diffval & 0xffff) > 15 || (diffval & 0xff000000) == 0) {
							goto not_flat;
						}
					}
				}
				touchpx[i] = 0xff000000 | (0x80 - dist) | (((diffpx[i] + 0x80) & 0xff) << 8);
				blobpx[i] = 0xff000000 | (0x80 - dist) | (((diffpx[i] + 0x80) & 0xff) << 8);
	not_flat:
				;
			}
		}

		/* Finally, find blobs in the labelled image. */
		vector<FingerTouch> newTouches = touchTrackingConnectedComponents(touchpx);
		vector<FingerTouch> curTouches = touches;
		{
			ofScopedLock lock(touchLock);
			touches = mergeTouches(curTouches, newTouches);
			touchesUpdated = true;
		}
	}
}

void OldIRDepthTouchTracker::drawDebug(float x, float y) {
	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();

	diffimage.reloadTexture();
	blobviz.reloadTexture();
	touchviz.reloadTexture();
	irCanny.flagImageChanged();
	irCanny.updateTexture();

	diffimage.draw(x, y);
	irCanny.draw(x, y+dh);
	blobviz.draw(x+dw*2, y);
	touchviz.draw(x+dw*2, y+dh);
}

/* update() function called from the main thread */
bool OldIRDepthTouchTracker::update(vector<FingerTouch> &retTouches) {
	fps.tick();

	{
		ofScopedLock lock(touchLock);
		if(touchesUpdated) {
			retTouches = touches;
			touchesUpdated = false;
			return true;
		} else {
			return false;
		}
	}
}

OldIRDepthTouchTracker::~OldIRDepthTouchTracker() {
	stopThread();
	waitForThread();
}

OldIRDepthTouchTracker::OldIRDepthTouchTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background)
: TouchTracker(depthStream, irStream, background) {
	diffimage.allocate(w, h, OF_IMAGE_COLOR_ALPHA);
	irCanny.allocate(w, h);
	blobviz.allocate(w, h, OF_IMAGE_COLOR_ALPHA);
	touchviz.allocate(w, h, OF_IMAGE_COLOR_ALPHA);

	nextTouchId = 1;
}
