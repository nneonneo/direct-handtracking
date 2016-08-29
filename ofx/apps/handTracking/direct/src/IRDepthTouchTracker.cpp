//
//  IRDepthTouchTracker.cpp
//  Infrared + Depth sensor-fusion touch tracker.
// 
//  Created by Robert Xiao on April 8, 2015.
//
//

#include "IRDepthTouchTracker.h"
#include "TextUtils.h"

/* Tweakable parameters */
/// Edge/fence parameters
const int edge_depthrel_dist = 2;	// px: distance range to consider relative-depth (smoothness) fence
const int edge_depthrel_thresh = 50; // mm: max diff between pixel and pixels in dist range
const int edge_depthabs_dist = 3;	// px: distance range to consider absolute-depth (height) fence
const int edge_depthabs_thresh = 100; // mm: max diff between pixels and bg in dist range

/// z/diff conditions for each of the four zones. z = z-value (relative to bg mean+stdev), diff = mm difference
#define ZONE_ERROR_COND (diff < -10)
#define ZONE_NOISE_COND (z < 0.7)
#define ZONE_LOW_COND (diff < 12)
#define ZONE_MID_COND (diff < 60)
// remaining pixels => ZONE_HIGH

/// object measurement parameters (n.b. ideally these would be in mm. Since they are in px, they should be
/// liberally set to avoid rejecting too many objects.)
const int arm_min_size = 100; // px: minimum size of arm blob
const int hand_min_size = 10; // px: minimum size of hand blob
const int finger_min_size = 10; // px: minimum size of finger blob
const int finger_min_dist = 5; // px: finger minimum length (distance); shorter "fingers" are pruned
const int tip_max_dist = 30; // px: maximum flood distance for tip fill (if exceeded, rollback)

/// postprocessing settings
// n.b. no smoothing for tip now (it is very stable with IR data)
const float smooth_tip_alpha = 1.0; // higher = less smoothing
const float smooth_touchz_alpha = 0.5;

const int tipavg_window = 1; // number of highest-distance pixels to average for tip position averaging
const int touchz_window = 8; // number of highest-distance pixels to average for touchz detection
const float touchz_enter = 0.5; // below this avg. z, a touch is considered active
const float touchz_exit = 2.5; // above this avg. z, a touch is considered inactive (must be higher than touchz_enter)

/* diffPx operators and definitions */
#define ZONE(x) ((x) & 0xffff0000)
#define DIFF(x) ((x) & 0x0000ffff)
#define ZONE_ERROR 0x00000000
#define ZONE_NOISE 0xff000000
#define ZONE_LOW   0xff400000
#define ZONE_MID   0xff800000
#define ZONE_HIGH  0xffc00000

#define BLOB_REJECTED 0x00020000
#define BLOB_VISITED 0x00010000

#pragma region Edge Map
void IRDepthTouchTracker::buildEdgeImage() {
	const int n = w * h;
	
	uint32_t *diffPx = (uint32_t *)diffIm[front].getPixels();
	uint32_t *edgePx = (uint32_t *)edgeIm[front].getPixels();
	fill_n(edgePx, n, 0);

	/* Build IR canny map */
	uint16_t *irPx = irStream.getPixelsRef().getPixels();
	uint8_t *ircannyPx = irCanny.getPixels();
	for(int i=0; i<n; i++) {
		ircannyPx[i] = irPx[i] / 64;
	}
	cv::Mat irCannyMat(irCanny.getCvImage());
	/* Edge finding, lightly tuned parameters */
	cv::Canny(irCannyMat, irCannyMat, 4000, 8000, 7, true);

	/* Mark significant pixels (IR pixels that will be holefilled). */
	/* Currently, all pixels are considered significant. */
	for(int i=0; i<n; i++) {
		if(ircannyPx[i] == 255)
			ircannyPx[i] = 224; // significant value
	}

	fillIrCannyHoles();

	/* Build final edge map */
	for(int i=0; i<n; i++) {
		if(ircannyPx[i])
			edgePx[i] |= 0xff000000 | (ircannyPx[i] << 16);
	}

	/* Depth relative edges */
	{
		const int WIN = edge_depthrel_dist; // flatness check window size
		for(int i=w*WIN; i<n-w*WIN; i++) {
			int myval = DIFF(diffPx[i]);
			for(int dy=-1; dy<=1; dy++) {
				for(int dx=-1; dx<=1; dx++) {
					int diffval = diffPx[i+dx*WIN+dy*WIN*w];
					// Reject if the other pixel differs greatly in diff value
					if(abs(myval - DIFF(diffval)) > edge_depthrel_thresh) {
						goto add_depthedge;
					}
				}
			}
			continue;
	add_depthedge:
			edgePx[i] |= 0xff00ff00;
		}
	}

	/* Depth absolute edges */
	{
		const int WIN = edge_depthabs_dist; // flatness check window size
		for(int i=w*WIN; i<n-w*WIN; i++) {
			/* Check that nearby pixels are also near the background.
			This check eliminates gradiated pixels on the edges of arms, knuckles, etc. */
			for(int dy=-1; dy<=1; dy++) {
				for(int dx=-1; dx<=1; dx++) {
					int diffval = diffPx[i+dx*WIN+dy*WIN*w];
					// Reject if the other pixel is too high off the ground
					if(DIFF(diffval) > edge_depthabs_thresh) {
						goto add_depthabs;
					}
				}
			}
			continue;
	add_depthabs:
			edgePx[i] |= 0xff0000ff;
		}
	}
}

void IRDepthTouchTracker::fillIrCannyHoles() {
	const int n = w * h;
	/* 255 = insignificant canny
	   224 = unvisited significant canny
	   208 = seen, unvisited significant canny
	   192 = visited significant canny
	   160 = fill candidate
	   128 = filled significant canny
	   0 = no canny */
	uint8_t *ircannypx = irCanny.getPixels();

	/* Find significant pixels and fill outwards */
	queue<int> queue;

	for(int idx=0; idx<n; idx++) {
		if(ircannypx[idx] != 224)
			continue;

		queue.push(idx);

		ircannypx[idx] = 208;
		while(!queue.empty()) {
			int curidx = queue.front();
			queue.pop();
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
				queue.push(otheridx); \
				ircannypx[otheridx] = 208; \
			} \
			} while(0)
			/* Eight-way neighbours, to cross diagonals */
			TEST(-1,-1);TEST(-1,0);TEST(-1,1);TEST(0,-1);TEST(0,1);TEST(1,-1);TEST(1,0);TEST(1, 1);
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
				queue.push(otheridx); \
				ircannypx[otheridx] = 160; \
			} \
			} while(0)
			/* Eight-way neighbours, to cross diagonals */
			TEST(-1,-1);TEST(-1,0);TEST(-1,1);TEST(0,-1);TEST(0,1);TEST(1,-1);TEST(1,0);TEST(1, 1);
#undef TEST
			}
		}
	}
}
#pragma endregion

void IRDepthTouchTracker::buildDiffImage() {
	const int n = w * h;

	uint16_t *depthPx = depthStream.getPixelsRef().getPixels();
	uint32_t *diffPx = (uint32_t *)diffIm[front].getPixels();

	const float *bgmean = background.getBackgroundMean().getPixels();
	const float *bgstdev = background.getBackgroundStdev().getPixels();

	/* Update diff image */
	for(int i=0; i<n; i++) {
		/* Update diff image */
		float diff;
		float z;
		if(depthPx[i]) {
			diff = bgmean[i] - depthPx[i];
			z = diff / bgstdev[i];
		} else {
			diff = 0;
			z = 0;
		}
		// A=valid B=zone GR=diff
		if(bgmean[i] == 0 || ZONE_ERROR_COND) diffPx[i] = ZONE_ERROR;
		else if(ZONE_NOISE_COND)diffPx[i] = ZONE_NOISE | (uint16_t)abs(diff);
		else if(ZONE_LOW_COND)	diffPx[i] = ZONE_LOW | (uint16_t)diff;
		else if(ZONE_MID_COND)	diffPx[i] = ZONE_MID | (uint16_t)diff;
		else					diffPx[i] = ZONE_HIGH | (uint16_t)diff;
	}
}

#pragma region Flood Filling
void IRDepthTouchTracker::rejectBlob(const vector<unsigned> &blob, int reason) {
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	for(auto i : blob) {
		blobPx[i & 0xffffff] |= BLOB_REJECTED | ((reason & 0x3f) << 18);
	}
}

static int colorForBlobIndex(int blobId) {
	/* Reverse the bits of the blob ID to make adjacent blob IDs more obvious */

	// https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
	unsigned char b = (unsigned char)blobId;
	b = ((b * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;
	return b;
}

vector<IRDepthArm> IRDepthTouchTracker::detectTouches() {
	const int n = w * h;

	nextBlobId = 1;

	vector<IRDepthArm> arms;
	uint32_t *diffPx = (uint32_t *)diffIm[front].getPixels();
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	fill_n(blobPx, n, 0);

	for(int i=0; i<n; i++) {
		if(ZONE(diffPx[i]) != ZONE_HIGH)
			continue;
		if(blobPx[i] != 0)
			continue;

		IRDepthArm arm;
		if(floodArm(arm, i)) {
			arms.push_back(arm);
		}
	}
	return arms;
}

bool IRDepthTouchTracker::floodArm(IRDepthArm &arm, unsigned idx) {
	const int n = w * h;

	uint32_t *diffPx = (uint32_t *)diffIm[front].getPixels();
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	vector<unsigned> q, q2;
	int qtail = 0;

	q.push_back(idx);

	while(qtail < q.size()) {
		unsigned curidx = q[qtail++];

		blobPx[curidx] |= ZONE_HIGH;

		int y = curidx / w;
		int x = curidx % w;

#define TEST(dx,dy) do {\
			int xx=x+dx, yy=y+dy; \
			if(0 <= xx && xx < w && 0 <= yy && yy < h) { \
				int otheridx = curidx + dy*w + dx; \
				if(blobPx[otheridx] != 0) \
					continue; \
				if(ZONE(diffPx[otheridx]) == ZONE_HIGH) \
					q.push_back(otheridx); \
				else if(ZONE(diffPx[otheridx]) == ZONE_MID) \
					q2.push_back(otheridx); \
				blobPx[otheridx] |= BLOB_VISITED; \
			} \
		} while(0)
		// four-way connectivity
		TEST(-1,0);TEST(0,-1);TEST(0,1);TEST(1,0);
#undef TEST
	}

	if(q.size() < arm_min_size) {
		/* Not enough pixels */
		rejectBlob(q, 1);
		return false;
	}

	/* Enough pixels for the arm: onto the next stage! */
	for(auto i : q2) {
		blobPx[i] &= ~BLOB_VISITED;
	}

	bool found_hands = false;
	for(auto i : q2) {
		if(blobPx[i] != 0)
			continue;
		IRDepthHand hand;
		if(floodHand(hand, i)) {
			arm.hands.push_back(hand);
			found_hands = true;
		}
	}

	if(!found_hands) {
		rejectBlob(q, 2);
		return false;
	}

	int color = colorForBlobIndex(nextBlobId++) << 8;
	for(auto i : q) {
		blobPx[i] |= color;
	}
	return true;
}

bool IRDepthTouchTracker::floodHand(IRDepthHand &hand, unsigned idx) {
	const int n = w * h;

	uint32_t *diffPx = (uint32_t *)diffIm[front].getPixels();
	uint32_t *edgePx = (uint32_t *)edgeIm[front].getPixels();
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	vector<unsigned> q, q2;
	int qtail = 0;

	q.push_back(idx);

	while(qtail < q.size()) {
		unsigned curidx = q[qtail++];

		blobPx[curidx] |= ZONE_MID;

		int y = curidx / w;
		int x = curidx % w;

#define TEST(dx,dy) do {\
			int xx=x+dx, yy=y+dy; \
			if(0 <= xx && xx < w && 0 <= yy && yy < h) { \
				int otheridx = curidx + dy*w + dx; \
				if(blobPx[otheridx] != 0) \
					continue; \
				if(edgePx[otheridx] & 0x00ffff00) /* IR + depth edge */\
					continue; \
				if(ZONE(diffPx[otheridx]) >= ZONE_MID) \
					q.push_back(otheridx); \
				else if(ZONE(diffPx[otheridx]) == ZONE_LOW) \
					q2.push_back(otheridx); \
				blobPx[otheridx] |= BLOB_VISITED; \
			} \
		} while(0)
		// four-way connectivity
		TEST(-1,0);TEST(0,-1);TEST(0,1);TEST(1,0);
#undef TEST
	}

	if(q.size() < hand_min_size) {
		/* Not enough pixels */
		rejectBlob(q, 3);
		return false;
	}

	/* Enough pixels for the hand: onto the next stage! */
	for(auto i : q2) {
		blobPx[i] &= ~BLOB_VISITED;
	}

	bool found_fingers = false;
	for(auto i : q2) {
		if(blobPx[i] != 0)
			continue;
		IRDepthFinger finger;
		if(floodFinger(finger, i)) {
			hand.fingers.push_back(finger);
			found_fingers = true;
		}
	}

	if(!found_fingers) {
		rejectBlob(q, 4);
		return false;
	}

	int color = colorForBlobIndex(nextBlobId++) << 8;
	for(auto i : q) {
		blobPx[i] |= color;
	}
	return true;
}

bool IRDepthTouchTracker::floodFinger(IRDepthFinger &finger, unsigned idx) {
	const int n = w * h;

	uint32_t *diffPx = (uint32_t *)diffIm[front].getPixels();
	uint32_t *edgePx = (uint32_t *)edgeIm[front].getPixels();
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	vector<unsigned> q, q2;
	vector<unsigned> roots; // pixels next to mid/high conf pixels
	int qtail = 0;

	q.push_back(idx);

	while(qtail < q.size()) {
		unsigned curidx = q[qtail++];
		unsigned dist = curidx >> 24;
		curidx &= 0xffffff;

		int y = curidx / w;
		int x = curidx % w;

		blobPx[curidx] |= ZONE_LOW | dist;

		bool isRoot = false; // are we adjacent to a mid/highconf pixel?

#define TEST(dx,dy) do {\
			int xx=x+dx, yy=y+dy; \
			if(0 <= xx && xx < w && 0 <= yy && yy < h) { \
				int otheridx = curidx + dy*w + dx; \
				if(blobPx[otheridx] >= ZONE_MID) \
					isRoot = true; \
				if(blobPx[otheridx] != 0) \
					continue; \
				if(edgePx[otheridx] & 0x00ff00ff) /* IR + depth abs */\
					continue; \
				if(ZONE(diffPx[otheridx]) >= ZONE_LOW) \
					q.push_back(otheridx | ((dist+1)<<24)); \
				else if(ZONE(diffPx[otheridx]) == ZONE_NOISE) \
					q2.push_back(otheridx | ((dist+1)<<24)); \
				blobPx[otheridx] |= BLOB_VISITED; \
			} \
		} while(0)
		// four-way connectivity
		TEST(-1,0);TEST(0,-1);TEST(0,1);TEST(1,0);
#undef TEST

		if(isRoot) {
			roots.push_back(curidx);
		}
	}

	/* Enough pixels for the finger: onto the next stage! */
	for(auto i : q2) {
		blobPx[i & 0xffffff] &= ~BLOB_VISITED;
	}

	vector<unsigned> tipq;

	for(auto i : q2) {
		if(blobPx[i & 0xffffff] != 0)
			continue;
		IRDepthTip tip;
		if(floodTip(tip, i)) {
			for(int j : tip.pixels) {
				q.push_back(j);
				tipq.push_back(j);
			}
			for(int j : tip.roots) {
				roots.push_back(j);
			}
		}
	}
	
	if(q.size() < finger_min_size) {
		/* Not enough pixels */
		rejectBlob(q, 5);
		for(auto i : tipq) {
			blobPx[i & 0xffffff] = 0;
		}
		return false;
	}

	refloodFinger(q, roots);

	if(!computeFingerMetrics(finger, q)) {
		/* Finger not really a finger */
		rejectBlob(q, 6);
		for(auto i : tipq) {
			blobPx[i & 0xffffff] = 0;
		}
		return false;
	}

	int color = colorForBlobIndex(nextBlobId++) << 8;
	for(auto i : q) {
		unsigned idx = i & 0xffffff;
		unsigned dist = i >> 24;

		blobPx[i & 0xffffff] |= color;
	}

	return true;
}

void IRDepthTouchTracker::refloodFinger(const vector<unsigned> &blob, vector<unsigned> &roots) {
	uint32_t *diffPx = (uint32_t *)diffIm[front].getPixels();
	uint32_t *edgePx = (uint32_t *)edgeIm[front].getPixels();
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	set<unsigned> unseen;
	for(auto i : blob)
		unseen.insert(i & 0xffffff);
	for(auto i : roots)
		unseen.erase(i);

	vector<unsigned> q = roots;
	int qtail = 0;
	while(qtail < q.size()) {
		unsigned curidx = q[qtail++];
		unsigned dist = curidx >> 24;
		curidx &= 0xffffff;

		blobPx[curidx] = (blobPx[curidx] & ~0xff) | dist;

#define TEST(dx,dy) do {\
			int otheridx = curidx + dy*w + dx; \
			if(unseen.count(otheridx)) { \
				q.push_back(otheridx | ((dist+1)<<24)); \
				unseen.erase(otheridx); \
			} \
		} while(0)
		// four-way connectivity
		TEST(-1,0);TEST(0,-1);TEST(0,1);TEST(1,0);
#undef TEST
	}
}

bool IRDepthTouchTracker::computeFingerMetrics(IRDepthFinger &finger, vector<unsigned> &px) {
	uint16_t *depthPx = depthStream.getPixelsRef().getPixels();
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	const float *bgmean = background.getBackgroundMean().getPixels();

	/* Sort pixels by distance */
	sort(px.begin(), px.end(), [&](unsigned a, unsigned b) { return (blobPx[a & 0xffffff] & 0xff) < (blobPx[b & 0xffffff] & 0xff); });

	/* Check max distance */
	int maxidx = px[px.size()-1] & 0xffffff;
	int maxdist = blobPx[maxidx] & 0xff;
	if(maxdist < finger_min_dist)
		return false;

	/* Average the z-heights */
	int start = px.size() - touchz_window;
	if(start < 0) start = 0;

	float avgdiff = 0;
	int count = 0;
	int idx;
	for(int i=start; i<px.size(); i++) {
		idx = px[i] & 0xffffff;
		avgdiff += bgmean[idx] - depthPx[idx];
		count++;
	}
	finger.z = avgdiff / count;

	/* Average the tip x and y values */
	start = px.size() - tipavg_window;
	if(start < 0) start = 0;

	float avgx = 0, avgy = 0;
	count = 0;
	for(int i=start; i<px.size(); i++) {
		idx = px[i] & 0xffffff;
		avgx += idx % w;
		avgy += idx / w;
		count++;
	}

	finger.x = avgx / count;
	finger.y = avgy / count;
	return true;
}

bool IRDepthTouchTracker::floodTip(IRDepthTip &tip, unsigned idx) {
	const int n = w * h;
	
	unsigned initial_dist = idx >> 24;

	uint32_t *diffPx = (uint32_t *)diffIm[front].getPixels();
	uint32_t *edgePx = (uint32_t *)edgeIm[front].getPixels();
	uint32_t *blobPx = (uint32_t *)blobIm[front].getPixels();

	vector<unsigned> q;
	int qtail = 0;

	q.push_back(idx);

	while(qtail < q.size()) {
		unsigned curidx = q[qtail++];
		unsigned dist = curidx >> 24;
		curidx &= 0xffffff;
		if(dist > tip_max_dist)
			goto reject_blob;

		int y = curidx / w;
		int x = curidx % w;

		blobPx[curidx] |= ZONE_NOISE | dist;

		bool isRoot = false; // are we adjacent to a mid/highconf pixel?

#define TEST(dx,dy) do {\
			int xx=x+dx, yy=y+dy; \
			if(0 <= xx && xx < w && 0 <= yy && yy < h) { \
				int otheridx = curidx + dy*w + dx; \
				if(blobPx[otheridx] >= ZONE_MID) \
					isRoot = true; \
				if(blobPx[otheridx] != 0) \
					continue; \
				if(edgePx[otheridx] & 0x00ff0000) /* IR only */\
					continue; \
				q.push_back(otheridx | ((dist+1) << 24)); \
				blobPx[otheridx] |= BLOB_VISITED; \
			} \
		} while(0)
		// four-way connectivity
		TEST(-1,0);TEST(0,-1);TEST(0,1);TEST(1,0);
#undef TEST

		if(isRoot) {
			tip.roots.push_back(curidx);
		}
	}

	swap(tip.pixels, q);
	return true;

reject_blob:
	for(auto i : q) {
		// We pretend that this blob never happened.
		blobPx[i & 0xffffff] = 0;
	}
	return false;
}
#pragma endregion

vector<FingerTouch> IRDepthTouchTracker::mergeTouches(vector<FingerTouch> &curTouches, vector<FingerTouch> &newTouches) {
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
            if(d > 50)
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
		/* EWMA new touch */
		newTouch.tip = smooth_tip_alpha * (newTouch.tip - curTouch.tip) + curTouch.tip;
		newTouch.touchZ = smooth_touchz_alpha * (newTouch.touchZ - curTouch.touchZ) + curTouch.touchZ;

		if(curTouch.touched && newTouch.touchZ > touchz_exit) {
			newTouch.touched = false;
            newTouch.statusAge = 0;
		} else if(!curTouch.touched && newTouch.touchZ < touchz_enter) {
			newTouch.touched = true;
            newTouch.statusAge = 0;
		} else {
			newTouch.touched = curTouch.touched;
			newTouch.statusAge = curTouch.statusAge + 1;
		}
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

void IRDepthTouchTracker::threadedFunction() {
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

		buildDiffImage();
		buildEdgeImage(); // edge image depends on diff

		vector<IRDepthArm> arms = detectTouches();

		vector<FingerTouch> newTouches;
		for(const IRDepthArm &arm : arms) {
			for(const IRDepthHand &hand : arm.hands) {
				for(const IRDepthFinger &finger : hand.fingers) {
					FingerTouch touch;
					touch.tip.set(finger.x, finger.y);
					touch.touchZ = finger.z;
					newTouches.push_back(touch);
				}
			}
		}

		vector<FingerTouch> curTouches = touches;
		{
			ofScopedLock lock(touchLock);
			touches = mergeTouches(curTouches, newTouches);
			touchesUpdated = true;
		}

		front = !front;
	}
}

void IRDepthTouchTracker::drawDebug(float x, float y) {
	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();

	int back = !front;

	diffIm[back].reloadTexture();
	edgeIm[back].reloadTexture();
	blobIm[back].reloadTexture();

	diffIm[back].draw(x, y);
	edgeIm[back].draw(x, y+dh);
	diffIm[back].draw(x+dw, y);
	edgeIm[back].draw(x+dw, y);

	blobIm[back].draw(x+dw, y+dh);
	
	drawText("Diff", x, y, HAlign::left, VAlign::top);
	drawText("Edge", x, y+dh, HAlign::left, VAlign::top);
	drawText("Diff+Edge", x+dw, y, HAlign::left, VAlign::top);
	drawText("Blob", x+dw, y+dh, HAlign::left, VAlign::top);
}

/* update() function called from the main thread */
bool IRDepthTouchTracker::update(vector<FingerTouch> &retTouches) {
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

IRDepthTouchTracker::~IRDepthTouchTracker() {
	stopThread();
	waitForThread();
}

IRDepthTouchTracker::IRDepthTouchTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background)
: TouchTracker(depthStream, irStream, background) {
	front = 0;

	for(int i=0; i<2; i++) {
		diffIm[i].allocate(w, h, OF_IMAGE_COLOR_ALPHA);
		edgeIm[i].allocate(w, h, OF_IMAGE_COLOR_ALPHA);
		blobIm[i].allocate(w, h, OF_IMAGE_COLOR_ALPHA);
	}
	irCanny.allocate(w, h);
}
