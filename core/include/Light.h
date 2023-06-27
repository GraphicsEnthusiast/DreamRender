#pragma once

#include <Utils.h>
#include <Material.h>
#include <Shape.h>

struct LightSample {
	vec3 light_dir;
	float light_pdf;
	float dist;
	vec3 light_normal;
	vec3 radiance;
};

struct HDRSample {
	vec3 L;
	vec3 radiance;
	float pdf;
};

class Light {
public:
	Light(Shape* s) : shape(s) {}

	virtual LightSample Sample(const IntersectionInfo& info, vec2 sample) = 0;

public:
	Shape* shape;
};

class SphereLight : public Light {
public:
	SphereLight(Sphere* s) : sphere(s), Light(s) {}

	virtual LightSample Sample(const IntersectionInfo& info, vec2 sample) override;

public:
	Sphere* sphere;
};

class QuadLight : public Light {
public:
	QuadLight(Quad* q) : quad(q), Light(q) {}

	virtual LightSample Sample(const IntersectionInfo& info, vec2 sample) override;

public:
	Quad* quad;
};

class Piecewise1D {
public:
	Piecewise1D() = default;
	Piecewise1D(const vector<float>& distrib);

	int Sample(const vec2& u);
	inline float Sum() const { return sumDistrib; }
	inline vector<pair<int, float>> GetTable() const { return table; }

private:
	typedef pair<int, float> Element;

private:
	vector<Element> table;
	float sumDistrib;
};

class PiecewiseIndependent2D {
public:
	PiecewiseIndependent2D() = default;
	PiecewiseIndependent2D(float* pdf, int width, int height);

	pair<int, int> Sample(const vec2& u1, const vec2& u2);
	inline float Sum() const { return colTable.Sum(); }

private:
	vector<Piecewise1D> rowTables;//行
	Piecewise1D colTable;//列
};

class InfiniteAreaLight {
public:
	InfiniteAreaLight(shared_ptr<HdrTexture> h, float scl = 1.0f);

	vec3 Emitted(const vec3& dir);
	HDRSample Sample(const IntersectionInfo& info, vec4 sample);
	float Pdf(const vec3& L, const vec3& N);
	float GetPortion(const vec3& L);

	inline float Power() { return mDistrib.Sum(); }

private:
	float scale;
	shared_ptr<HdrTexture> hdr;
	PiecewiseIndependent2D mDistrib;
};

inline vec3 PlaneToSphere(const vec2& uv) {
	vec2 xy = uv;
	xy.y = 1.0f - xy.y; //flip y

	//获取角度
	float phi = 2.0f * PI * (xy.x - 0.5f);    //[-pi ~ pi]
	float theta = PI * (xy.y - 0.5f);        //[-pi/2 ~ pi/2]   

	//球坐标计算方向
	vec3 L = vec3(cos(theta) * cos(phi), sin(theta), cos(theta) * sin(phi));

	return L;
}

inline vec2 SphereToPlane(const vec3& v) {
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv /= vec2(2.0f * PI, PI);
	uv += 0.5f;
	uv.y = 1.0f - uv.y;

	return uv;
}
