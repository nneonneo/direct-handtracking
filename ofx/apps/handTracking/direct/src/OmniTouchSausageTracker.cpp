//
//  OmniTouchSausageTracker.cpp
//  Implementation of OmniTouch's sausage-finding touch tracker.
// 
//  Created by Robert Xiao on April 9, 2015.
//
//

#include "OmniTouchSausageTracker.h"
#include "TextUtils.h"

static uint8_t clamp(int val) {
    if(val < 0) return 0;
    if(val > 255) return 255;
    return val;
}

static const int MAX_CUTOFF = 1800; // mm
static void calc_depth_dx(int W, int H, unsigned char *dxpx, const unsigned short *depthpx, const int diff_dist, const int num_channels) {
	int prevback, prevfront;
    for(int y=0; y<H; y++) {
        for(int x=0; x<diff_dist; x++) {
            *dxpx = 127;
            dxpx += num_channels;
        }
		prevback = prevfront = 0;
        for(int x=diff_dist; x<W; x++) {
            int back = depthpx[x-diff_dist];
            int front = depthpx[x];
			if((!back || back > MAX_CUTOFF) && (!front || front > MAX_CUTOFF)) {
				*dxpx = 0;
			} else {
				if(!back || back > MAX_CUTOFF)
					back = prevback;
				if(!front || front > MAX_CUTOFF)
					front = prevfront;

				if(back == 0 || front == 0)
					*dxpx = 0;
				else
					*dxpx = clamp((front - back) + 127);
			}
			dxpx += num_channels;
			prevback = back;
			prevfront = front;
        }
        depthpx += W;
    }
}

static void calc_depth_dy(int W, int H, unsigned char *dypx, const unsigned short *depthpx, const int diff_dist, const int num_channels) {
	int prevback, prevfront;
    for(int y=0; y<diff_dist; y++) {
        for(int x=0; x<W; x++) {
            *dypx = 127;
            dypx += num_channels;
        }
        depthpx += W;
    }
    for(int y=diff_dist; y<H; y++) {
		prevback = prevfront = 0;
        for(int x=0; x<W; x++) {
            int back = depthpx[x-diff_dist*W];
            int front = depthpx[x];
			if((!back || back > MAX_CUTOFF) && (!front || front > MAX_CUTOFF)) {
				*dypx = 0;
			} else {
				if(!back || back > MAX_CUTOFF)
					back = prevback;
				if(!front || front > MAX_CUTOFF)
					front = prevfront;

				if(back == 0 || front == 0)
					*dypx = 0;
				else
					*dypx = clamp((front - back) + 127);
			}
			dypx += num_channels;
			prevback = back;
			prevfront = front;
        }
        depthpx += W;
    }
}

struct SausageFinder {
    const static uint8_t FLAG_VISITED_X = 1; // flag for visited x
    const static uint8_t FLAG_VISITED_Y = 2; // flag for visited y
    
    const static int ssx_shift = 16; // blue channel
    const static int ssf_shift = 8; // green channel
    const static int ssy_shift = 0;  // red channel
    
    int SEARCH_GAP; // allowed pixel gap between adjacent slices
    int MIN_SLICES; // minimum number of slices
    
    /* Slice search parameters */
    int X_ENTER_MIN;
    int X_ENTER_MAX;
    int X_EXIT_MIN;
    int X_EXIT_MAX;
    int X_WIDTH_MIN;
    int X_WIDTH_MAX;
    
    int Y_ENTER_MIN;
    int Y_ENTER_MAX;
    int Y_EXIT_MIN;
    int Y_EXIT_MAX;
    int Y_WIDTH_MIN;
    int Y_WIDTH_MAX;
    
    int W, H;

    uint32_t *sspx_start, *sspx_end;
    SausageFinder(int W, int H, uint32_t *sspx)
    : W(W), H(H) {

		SEARCH_GAP = 3; // allowed pixel gap between adjacent slices
		MIN_SLICES = 8; // minimum number of slices
    
		/* Slice search parameters */
		X_ENTER_MIN = 127 - 57;
		X_ENTER_MAX = 127 - 5;
		X_EXIT_MIN = 127 + 5;
		X_EXIT_MAX = 127 + 57;
		X_WIDTH_MIN = 3;
		X_WIDTH_MAX = 6; // wide enough for 45-degree angles
    
		Y_ENTER_MIN = 127 - 30;
		Y_ENTER_MAX = 127 - 5;
		Y_EXIT_MIN = 127 + 5;
		Y_EXIT_MAX = 127 + 57;
		Y_WIDTH_MIN = 3;
		Y_WIDTH_MAX = 6; // wide enough for 45-degree angles
  
        sspx_start = sspx;
        sspx_end = sspx_start + W*H;
    }
    
    void find_x_slices(const unsigned char *dxpx, const int diff_channels) const {
        uint32_t *sspx = sspx_start;
        
        for(int y=0; y<H; y++) {
            for(int x=0; x<W; x++) {
                uint8_t dx_enter = dxpx[x*diff_channels];
                if(dx_enter < X_ENTER_MIN || dx_enter > X_ENTER_MAX)
                    continue;
                
                for(int dx = X_WIDTH_MIN; dx < X_WIDTH_MAX && x+dx < W; dx++) {
                    uint8_t dx_exit = dxpx[(x+dx)*diff_channels];
                    if(dx_exit == 0 || dxpx[(x+dx/2)*diff_channels] == 0)
                        break;
                    if(dx_exit < X_EXIT_MIN || dx_exit > X_EXIT_MAX)
                        continue;
                    
                    /* Found enter + exit pair */
                    /* Pixel format: [dx] [256-1] [256-2] [256-3] ... [256-dx+1] */
                    sspx[x] |= 0xff000000 | (dx << ssx_shift);
                    for(int i=1; i<dx; i++) {
                        sspx[x+i] |= 0xff000000 | ((256-i) << ssx_shift);
                    }
                    x += dx;
                    break;
                }
            }
            dxpx += W*diff_channels;
            sspx += W;
        }
    }
    
    void find_y_slices(const unsigned char *dypx, const int diff_channels) const {
        uint32_t *sspx = sspx_start;
        
        for(int x=0; x<W; x++) {
            for(int y=0; y<H; y++) {
                uint8_t dy_enter = dypx[y*W*diff_channels];
                if(dy_enter < Y_ENTER_MIN || dy_enter > Y_ENTER_MAX)
                    continue;
                
                for(int dy = Y_WIDTH_MIN; dy < Y_WIDTH_MAX && y+dy < H; dy++) {
                    uint8_t dy_exit = dypx[(y+dy)*W*diff_channels];
                    if(dy_exit == 0 || dypx[(y+dy/2)*W*diff_channels] == 0)
                        break;
                    if(dy_exit < Y_EXIT_MIN || dy_exit > Y_EXIT_MAX)
                        continue;
                    
                    /* Found enter + exit pair */
                    /* Pixel format: [dy] [256-1] [256-2] [256-3] ... [256-dy+1] */
                    sspx[y*W] |= 0xff000000 | (dy << ssy_shift);
                    for(int i=1; i<dy; i++) {
                        sspx[(y+i)*W] |= 0xff000000 | ((256-i) << ssy_shift);
                    }
                    y += dy;
                    break;
                }
            }
            dypx += diff_channels;
            sspx += 1;
        }
    }
    
private:
    uint32_t *midpt_x(uint32_t *sspx) const {
        uint8_t ssx = *sspx >> ssx_shift;
        if(ssx > 127) {
            sspx -= (256 - ssx);
            ssx = *sspx >> ssx_shift;
        }
        return sspx + ssx/2;
    }
    
    uint32_t *midpt_y(uint32_t *sspx) const {
        uint8_t ssy = *sspx >> ssy_shift;
        if(ssy > 127) {
            sspx -= (256 - ssy) * W;
            ssy = *sspx >> ssy_shift;
        }
        return sspx + ssy/2*W;
    }
    
    /* Find a finger composed of x-slices, starting from sspx (which must be a midpt point).
     sspx is assumed to have already been pushed onto points. */
    void find_x_finger(uint32_t *sspx, vector<uint32_t *> &points, bool can_switch, bool reverse=false) const {
        uint32_t *initial_sspx = sspx;
        
        while(1) {
            if(can_switch)
                *sspx |= FLAG_VISITED_X << ssf_shift;
            
            /* Look for next x slice */
            int found_x = 0;
            if(reverse) {
                for(int i=-1; i>-SEARCH_GAP && sspx+i*W >= sspx_start; i--) {
                    if((uint8_t)(sspx[i*W] >> ssx_shift)) {
                        found_x = i;
                        break;
                    }
                }
            } else {
                for(int i=1; i<SEARCH_GAP && sspx+i*W < sspx_end; i++) {
                    if((uint8_t)(sspx[i*W] >> ssx_shift)) {
                        found_x = i;
                        break;
                    }
                }
            }
            
            if(found_x) {
                /* Push on the newfound x */
                sspx = midpt_x(sspx + found_x*W);
                points.push_back(sspx);
                continue;
            } else if(can_switch) {
                /* Try switching. */
                int initial_x = (initial_sspx - sspx_start) % W;
                int current_x = (sspx - sspx_start) % W;
                find_y_finger(sspx, points, false, initial_x > current_x);
                break;
            } else {
                /* We're done here */
                break;
            }
        } // while(1)
    }
    
    /* Find a finger composed of y-slices, starting from sspx (which must be a midpt point).
     sspx is assumed to have already been pushed onto points. */
    void find_y_finger(uint32_t *sspx, vector<uint32_t *> &points, bool can_switch, bool reverse=false) const {
        uint32_t *initial_sspx = sspx;
        
        while(1) {
            if(can_switch)
                *sspx |= FLAG_VISITED_Y << ssf_shift;
            
            /* Look for next x slice */
            int found_y = 0;
            if(reverse) {
                /* Look for next x/y slice. */
                for(int i=-1; i>-SEARCH_GAP && sspx+i >= sspx_start; i--) {
                    if((uint8_t)(sspx[i] >> ssy_shift)) {
                        found_y = i;
                        break;
                    }
                }
            } else {
                for(int i=1; i<SEARCH_GAP && sspx+i < sspx_end; i++) {
                    if((uint8_t)(sspx[i] >> ssy_shift)) {
                        found_y = i;
                        break;
                    }
                }
            }
            
            if(found_y) {
                /* Push on the newfound y */
                sspx = midpt_y(sspx + found_y);
                points.push_back(sspx);
                continue;
            } else if(can_switch) {
                /* Try switching directions */
                int initial_y = (initial_sspx - sspx_start) / W;
                int current_y = (sspx - sspx_start) / W;
                find_x_finger(sspx, points, false, initial_y > current_y);
                break;
            } else {
                /* We're done here */
                break;
            }
        } // while(1)
    }
    
    void find_x_fingers(vector<vector<uint32_t *>> &fingers) const {
        vector<uint32_t *> points;
        
        for(int y=0; y<H; y++) {
            uint32_t *sspx_row = sspx_start + y*W;
            for(int x=0; x<W;) {
                uint32_t *sspx = sspx_row + x;
                if(!*sspx) {
                    x++;
                    continue;
                }
                
                uint8_t slicelen = *sspx >> ssx_shift;
                if(!slicelen) {
                    x++;
                    continue;
                }
                
                // sspx should always be the start of a slice
                assert(slicelen < 127);
                
                uint32_t *midpt = midpt_x(sspx);
                /* Going to move past this slice when we're done */
                x += slicelen;
                
                uint8_t ssf = *midpt >> ssf_shift;
                if(ssf & FLAG_VISITED_X) {
                    /* skip the slice */
                    continue;
                }
                
                points.clear();
                points.push_back(midpt);
                find_x_finger(midpt, points, true);
                
                /* See if the segment is valid */
                if(points.size() < MIN_SLICES)
                    continue; /* REJECT */
                
                fingers.push_back(points);
            }
        }
    }
    
    void find_y_fingers(vector<vector<uint32_t *>> &fingers) const {
        vector<uint32_t *> points;
        
        for(int x=0; x<W; x++) {
            uint32_t *sspx = sspx_start + x;
            for(int y=0; y<H;) {
                if(!*sspx) {
                    y++;
                    sspx += W;
                    continue;
                }
                
                uint8_t slicelen = *sspx >> ssy_shift;
                if(!slicelen) {
                    y++;
                    sspx += W;
                    continue;
                }
                
                // sspx should always be the start of a slice
                assert(slicelen < 127);
                
                uint32_t *midpt = midpt_y(sspx);
                /* Going to move past this slice when we're done */
                y += slicelen;
                sspx += slicelen*W;
                
                uint8_t ssf = *midpt >> ssf_shift;
                if(ssf & FLAG_VISITED_Y) {
                    /* skip the slice */
                    continue;
                }
                
                points.clear();
                points.push_back(midpt);
                find_y_finger(midpt, points, true);
                
                /* See if the segment is valid */
                if(points.size() < MIN_SLICES)
                    continue; /* REJECT */
                
                fingers.push_back(points);
            }
        }
    }
    
public:
    vector<vector<int>> find_fingers() const {
        vector<vector<int>> ret;
        vector<vector<uint32_t *>> fingers;
        
        find_x_fingers(fingers);
        find_y_fingers(fingers);
        
        for(const auto &points : fingers) {
            /* It's good! Mark the centers. */
            vector<int> point_ints;
            for(uint32_t *ss : points) {
                *ss |= 0xff << ssf_shift;
                point_ints.push_back(ss - sspx_start);
            }
            ret.push_back(point_ints);
        }
        
        return ret;
    }
};

vector<FingerTouch> OmniTouchSausageTracker::findTouches() {
    /* Image processing */
    const int diff_dist = 3;

	uint16_t *depthPx = depthStream.getPixelsRef().getPixels();
	uint32_t *sausagePx = (uint32_t *)sausageIm[front].getPixels();
	
	const float *bgmean = background.getBackgroundMean().getPixels();
	
    fill_n((uint32_t *)diffIm[front].getPixels(), w*h, 0xff000000);

    unsigned char *const dxpx_start = diffIm[front].getPixels() + 2; // blue channel
    unsigned char *const dypx_start = diffIm[front].getPixels() + 0; // red channel
    
    calc_depth_dx(w, h, dxpx_start, depthPx, diff_dist, 4);
    calc_depth_dy(w, h, dypx_start, depthPx, diff_dist, 4);
    
    fill_n(sausagePx, w*h, 0);
    SausageFinder finger_finder(w, h, sausagePx);
    finger_finder.find_x_slices(dxpx_start, 4);
    finger_finder.find_y_slices(dypx_start, 4);
    auto fingers = finger_finder.find_fingers();

    /* Construct candidate touches from fingers */
    vector<FingerTouch> touches;
    for(const auto &finger : fingers) {
		int idxTip = finger[2];
		int idxBase = finger[finger.size()-2];
        ofPoint tip(idxTip % w, idxTip / w);
        ofPoint base(idxBase % w, idxBase / w);

		if(depthPx[idxTip] < depthPx[idxBase]) {
			swap(tip, base);
			swap(idxTip, idxBase);
		}

		if(bgmean[idxTip] - depthPx[idxTip] < 7) {
			FingerTouch ft;
			ft.id = touches.size();
			ft.tip = tip;
			ft.base = base;
			ft.touched = true;
			touches.push_back(ft);
		}
    }

	return touches;
}

vector<FingerTouch> OmniTouchSausageTracker::filterTouches(vector<FingerTouch> &touches) {
    
    /* Filter out redundant/noisy touches */
    vector<FingerTouch> filtered_touches;
    for(const auto &ft : touches) {
        bool should_be_added = true;

        /* Check if touch is near any other touches */
        for(const auto &other : touches) {
            if(!should_be_added)
                break;

            if(&ft == &other)
                continue;
            
            int violations = 0;
            // Note: right now this checks only the tip
            for(int i=0; i<1; i++) {
                ofPoint pt = (i == 0) ? ft.tip : ft.base;
                
                ofVec2f pv = pt - other.base;
                ofVec2f tv = other.tip - other.base;
                ofVec2f tvn = tv.normalized();
                float r = pv.dot(tvn);
                if(r < -10 || r >= tv.length()+10) {
                    // OK: point lies outside the segment joining other.tip and other.base.
                    continue;
                }
                pv -= r*tvn;
                if(pv.length() > 9) {
                    // OK: point is perpendicularly more than 8 units away from the other segment.
                    continue;
                }
                // Not OK: point is inside the other finger's personal space.
                violations++;
            }
            
            if(violations == 0) {
                continue;
            } else if(violations == 1) {
                /* One violation. Pick the longer one. */
                float mylen = ft.tip.distance(ft.base);
                float otherlen = other.tip.distance(other.base);
                if(mylen < otherlen) {
                    should_be_added = false;
                } else if(mylen == otherlen && ft.id > other.id) {
                    // Same length: remove one of them
                    should_be_added = false;
                }
            } else {
                should_be_added = false;
            }
        }
        
        if(should_be_added) {
			/* Forward project the tip a few mm. */
			ofVec3f dir = ft.tip - ft.base;
			dir.normalize();
			FingerTouch newTouch = ft;
			newTouch.tip += dir * 4;
            filtered_touches.push_back(newTouch);
        }
    }

    return filtered_touches;
}

vector<FingerTouch> OmniTouchSausageTracker::mergeTouches(vector<FingerTouch> &curTouches, vector<FingerTouch> &newTouches) {
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

void OmniTouchSausageTracker::threadedFunction() {
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
		
		vector<FingerTouch> newTouches = filterTouches(findTouches());
		vector<FingerTouch> curTouches = touches;
		{
			ofScopedLock lock(touchLock);
			touches = mergeTouches(curTouches, newTouches);
			touchesUpdated = true;
		}

		front = !front;
	}
}

void OmniTouchSausageTracker::drawDebug(float x, float y) {
	const int dw = depthStream.getWidth();
	const int dh = depthStream.getHeight();

	int back = !front;

	diffIm[back].reloadTexture();
	sausageIm[back].reloadTexture();

	diffIm[back].draw(x, y);
	sausageIm[back].draw(x, y+dh);

	drawText("Diff", x, y, HAlign::left, VAlign::top);
	drawText("Sausage", x, y+dh, HAlign::left, VAlign::top);
}

/* update() function called from the main thread */
bool OmniTouchSausageTracker::update(vector<FingerTouch> &retTouches) {
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

OmniTouchSausageTracker::~OmniTouchSausageTracker() {
	stopThread();
	waitForThread();
}

OmniTouchSausageTracker::OmniTouchSausageTracker(ofxKinect2::DepthStream &depthStream, ofxKinect2::IrStream &irStream, BackgroundUpdaterThread &background)
: TouchTracker(depthStream, irStream, background) {
	front = 0;

	for(int i=0; i<2; i++) {
		diffIm[i].allocate(w, h, OF_IMAGE_COLOR_ALPHA);
		sausageIm[i].allocate(w, h, OF_IMAGE_COLOR_ALPHA);
	}
}
