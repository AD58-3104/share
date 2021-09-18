#pragma once
#ifndef HPL_TYPES_H_
#define HPL_TYPES_H_

#include <cmath>
#include <ctime>
#include <vector>
#include <algorithm>

enum Col_def_T {
	COLOR_BALL = 0,
	COLOR_YELLOW,
	COLOR_BLUE,
	COLOR_BLACK,
	COLOR_MAGENTA,
	COLOR_CYAN,
	COLOR_WHITE,
	COLOR_GREEN,
	COLOR_BLACK_ON_GREEN,
	COLOR_BLACK_ON_GREEN_ERASE_V_LINE
};

enum Object_T {
	OBJECT_BALL,
	OBJECT_GOAL,
	OBJECT_GOAL_POST,
	OBJECT_POLE,
	OBJECT_ROBOT,
	OBJECT_BLACK_POLE,
	OBJECT_WHITE_LINES,
	OBJECT_EST_BALL,
	OBJECT_NUM_TYPES,
	OBJECT_UNKNOWN = 99
};

enum Object_Ownership_T {
	OWNERSHIP_OURS = 0,
	OWNERSHIP_THEIRS,
	OWNERSHIP_ANY,
	OWNERSHIP_NUM_TYPES
};

enum HL_Size_Class_T {
	HL_CLASS_KID = 0,
	HL_CLASS_TEEN
};

#define HLMCL_FIELD_2013_KID  5
#define HLMCL_FIELD_2013_TEEN 6
#define HLMCL_FIELD_2013_SPL 7

struct Pos2D
{
	Pos2D() : x(0), y(0), th(0) {}
	Pos2D(float ix, float iy, float ith) : x(ix), y(iy), th(ith) {}
	Pos2D(double ix, double iy, double ith) : x((float)ix), y((float)iy), th((float)ith) {}
	float x;
	float y;
	float th;
	Pos2D GlobalToLocal(const Pos2D &self_pos) const {
		float dx = x - self_pos.x;
		float dy = y - self_pos.y;
		double r = std::sqrt(dx * dx + dy * dy);
		double phi = std::atan2(dy, dx);
		Pos2D fs;
		fs.x = (float)(r * std::cos(phi - self_pos.th));
		fs.y = (float)(r * std::sin(phi - self_pos.th));
		fs.th = th - self_pos.th;
		return fs;
	}
	Pos2D LocalToGlobal(const Pos2D &self_pos) const {
		float r = (float)std::sqrt(x*x + y*y);
		float phi = (float)std::atan2(y, x);
		Pos2D gl;
		gl.x  = (float)(self_pos.x + r * cos(self_pos.th + phi));
		gl.y  = (float)(self_pos.y + r * sin(self_pos.th + phi));
		gl.th = self_pos.th + th;
		return gl;
	}
	float distanceTo(const Pos2D &pos) const {
		return std::sqrt((pos.x - x)*(pos.x - x)+(pos.y - y)*(pos.y - y));
	}
};

struct PosMemory
{
	PosMemory() : detected(false), time(0) {
	}
	Pos2D pos;
	float time; //sec countup
	bool detected;
	float cf; //confidence: 0-100
};

struct Pos2DCf
{
	bool is_detect;
	Pos2D pos;
	float cf;
	time_t time;
	
	Pos2DCf() : is_detect(false), cf(0), time(0) {};
	Pos2DCf(const Pos2D &pos2d, float in_cf, time_t ctime): cf(in_cf), time(ctime) {
		pos = pos2d;
	}
	Pos2DCf(float ix, float iy, float ith, float icf, time_t itime)
		: is_detect(true), pos(ix, iy, ith), cf(icf), time(itime) {};
	int setPosition(float ix, float iy, float ith, float icf, time_t itime)
	{
		is_detect = true;
		pos.x = ix;
		pos.y = iy;
		pos.th = ith;
		cf = icf;
		time = itime;
		return 0;
	}
	int resetPosition()
	{
		is_detect = false;
		return 0;
	}
};

struct BoundingBox
{
	BoundingBox(): x(0), y(0), w(0), h(0), prob(0) {
	}
	BoundingBox(float ax, float ay, float aw, float ah, float aprob) : x(ax), y(ay), w(aw), h(ah), prob(aprob) {
	}
	bool operator<(const BoundingBox &b) const {
		return (this->prob > b.prob);
	}
	float getLeft(void) const
	{
		return x - w / 2.0;
	}
	float getRight(void) const
	{
		return x + w / 2.0;
	}
	float getTop(void) const
	{
		return y - h / 2.0;
	}
	float getBottom(void) const
	{
		return y + h / 2.0;
	}
	float x;
	float y;
	float w;
	float h;
	float prob;
};

namespace CitBrains {
	struct NPos2D // pos2D is for integer value(natural number pos in 2D)
	{
		NPos2D() : x(0), y(0){}
		NPos2D(int in_x, int in_y) : x(in_x), y(in_y){}
		int x;
		int y;
	};

	class WhiteLineMap
	{
	public:
		WhiteLineMap(int w, int h) : width(w), height(h), sx(-w/2), sy(-h/2), CELLSIZE(20), ncols(width / CELLSIZE), nrows(height / CELLSIZE), data(ncols * nrows, 0) {
		}
		void put(int x, int y) {
			const int ix = (x - sx) / CELLSIZE;
			const int iy = (y - sy) / CELLSIZE;
			if (ix < 0 || iy < 0 || ncols <= ix || nrows <= iy) {
				return;
			}
			data[iy * ncols + ix]++;
		}
		void clear() {
			std::fill(data.begin(), data.end(), 0);
		}
		const std::vector<int> &getRawData() const {
			return data;
		}
		std::vector<NPos2D> expand() {
			std::vector<NPos2D> wlobs;
			int mapind = 0;
			const std::vector<int> &maprawdata = getRawData();
			for(int y = 0; y < nrows; y++) {
				for(int x = 0; x < ncols; x++) {
					if(maprawdata[mapind++] > 0) {
						NPos2D p;
						p.x = x * CELLSIZE + sx;
						p.y = y * CELLSIZE + sy;
						wlobs.push_back(p);
					}
				}
			}
			return wlobs;
		}
	private:
		const int width;
		const int height;
		const int sx;
		const int sy;
		const int CELLSIZE; /* unit: milli meter */
		const int ncols;
		const int nrows;
		std::vector<int> data;
	};

	struct ball_particle_T {
		ball_particle_T() : x(0), y(0), radius(0), score(0.0) { }
		ball_particle_T(int xt, int yt, int radiust, float scoret) : x(xt), y(yt), radius(radiust), score(scoret) { }
		int x;
		int y;
		int radius;
		float score;
	};
}; // end of namespace CitBrains

#endif // HPL_TYPES_H_
