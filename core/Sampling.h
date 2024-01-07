#pragma once

#include "Utils.h"

class AliasTable1D {
public:
	AliasTable1D() = default;

	AliasTable1D(const std::vector<float>& distrib);

	int Sample(const Point2f& sample);

	inline float Sum() const { 
		return sumDistrib; 
	}

	inline std::vector<std::pair<int, float>> GetTable() const { 
		return table; 
	}

private:
	typedef std::pair<int, float> Element;

private:
	std::vector<Element> table;
	float sumDistrib;
};

class AliasTable2D {
public:
	AliasTable2D() = default;

	AliasTable2D(const std::vector<float>& distrib, int width, int height);

	std::pair<int, int> Sample(const Point2f& sample1, const Point2f& sample2);

	inline float Sum() const { 
		return colTable.Sum(); 
	}

private:
	std::vector<AliasTable1D> rowTables;
	AliasTable1D colTable;
};

inline Point2f UniformSampleDisk(const Point2f& sample, float radius) {
	float sampleY = sample.x;
	float sampleX = sample.y;

	float theta = 2.0f * PI * sampleX;
	float r = sampleY * radius;

	return Vector2f(r * std::cos(theta), r * std::sin(theta));
}

inline Vector3f PlaneToSphere(const Point2f& uv) {
	Point2f xy = uv;

	float phi = 2.0f * PI * (xy.x - 0.5f);// [-pi ~ pi]
	float theta = PI * (xy.y - 0.5f);// [-pi/2 ~ pi/2]   

	Vector3f L(cos(theta) * cos(phi), sin(theta), cos(theta) * sin(phi));

	return normalize(L);
}

inline Point2f SphereToPlane(const Vector3f& v) {
	Point2f uv(std::atan2(v.z, v.x), std::asin(v.y));
	uv *= Point2f(INV_2PI, INV_PI);
	uv += 0.5f;

	return uv;
}

inline Vector3f UniformSampleCone(const Point2f& sample, float cos_angle) {
	Vector3f p(0.0f);

	float phi = 2.0f * PI * sample.x;
	float cos_theta = glm::mix(cos_angle, 1.0f, sample.y);
	float sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);

	p.x = sin_theta * std::cos(phi);
	p.y = sin_theta * std::sin(phi);
	p.z = cos_theta;

	return normalize(p);
}

inline float UniformPdfCone(float cos_angle) {
	return INV_2PI /  (1.0f - cos_angle);
}

inline Vector3f CosineSampleHemisphere(const Point2f& sample) {
	Vector3f p(0.0f);

	// Uniformly sample disk
	float r = std::sqrt(sample.x);
	float phi = 2.0f * PI * sample.y;
	p.x = r * std::cos(phi);
	p.y = r * std::sin(phi);

	// Project up to hemisphere
	p.z = std::sqrt(std::max(0.0f, 1.0f - p.x * p.x - p.y * p.y));

	return normalize(p);
}

inline float CosinePdfHemisphere(float NdotL) {
	return NdotL * INV_PI;
}

inline Vector3f UniformSampleSphere(const Point2f& sample) {
	Vector3f p(0.0f);

	float phi = 2.0f * PI * sample.x;
	float cos_theta = 1.0f - 2.0f * sample.y;
	float sin_theta = sqrt(1.0f - cos_theta * cos_theta);

	p.x = sin_theta * cos(phi);
	p.y = sin_theta * sin(phi);
	p.z = cos_theta;

	return normalize(p);
}

inline float UniformPdfSphere() {
	return INV_4PI;
}

inline Vector3f UniformSampleHemisphere(const Point2f& sample) {
	float r = std::sqrt(std::max(0.0f, 1.0f - sample.x * sample.x));
	float phi = 2.0f * PI * sample.y;
	Vector3f p(r * cos(phi), r * sin(phi), sample.x);

	return normalize(p);
}

inline float UniformPdfHemisphere() {
	return INV_2PI;
}

template <typename Predicate>
inline int IntervalSearch(int size, const Predicate& pred) {
	int first = 0, len = size;
	while (len > 0) {
		int half = len >> 1, middle = first + half;
		// Bisect range based on value of _pred_ at _middle_
		if (pred(middle)) {
			first = middle + 1;
			len -= half + 1;
		}
		else {
			len = half;
		}
	}

	return glm::clamp(first - 1, 0, size - 2);
}

inline int IntervalSearch(int size, const float* array, float x) {
	int begin = 0;
	int end = size - 1;
	while ((end - begin) > 1) {
		int middle = (begin + end) / 2;
		if (x > array[middle]) {
			begin = middle;
		}
		else {
			end = middle;
		}
	}
	if (begin < 0) {
		return 0;
	}
	else if (begin > (size - 2)) {
		return size - 2;
	}
	else {
		return begin;
	}
}

inline float IntegrateCatmullRom(int n, const float* x, const float* values, float* cdf) {
	float sum = 0.0f;
	cdf[0] = 0.0f;
	for (int i = 0; i < n - 1; ++i) {
		// Look up $x_i$ and function values of spline segment _i_
		float x0 = x[i], x1 = x[i + 1];
		float f0 = values[i], f1 = values[i + 1];
		float width = x1 - x0;

		// Approximate derivatives using finite differences
		float d0, d1;
		if (i > 0) {
			d0 = width * (f1 - values[i - 1]) / (x1 - x[i - 1]);
		}
		else {
			d0 = f1 - f0;
		}
		if (i + 2 < n) {
			d1 = width * (values[i + 2] - f0) / (x[i + 2] - x0);
		}
		else {
			d1 = f1 - f0;
		}

		// Keep a running sum and build a cumulative distribution function
		sum += ((d0 - d1) * (1.0f / 12.0f) + (f0 + f1) * 0.5f) * width;
		cdf[i + 1] = sum;
	}

	return sum;
}

inline bool CatmullRomWeights(int size, const float* nodes, float x, int* offset, float* weights) {
	// Return _false_ if _x_ is out of bounds
	if (!(x >= nodes[0] && x <= nodes[size - 1])) {
		return false;
	}

	// Search for the interval _idx_ containing _x_
	int idx = IntervalSearch(size, [&](int i) { return nodes[i] <= x; });
	*offset = idx - 1;
	float x0 = nodes[idx], x1 = nodes[idx + 1];

	// Compute the $t$ parameter and powers
	float t = (x - x0) / (x1 - x0), t2 = t * t, t3 = t2 * t;

	// Compute initial node weights $w_1$ and $w_2$
	weights[1] = 2.0f * t3 - 3.0f * t2 + 1.0f;
	weights[2] = -2.0f * t3 + 3.0f * t2;

	// Compute first node weight $w_0$
	if (idx > 0) {
		float w0 = (t3 - 2.0f * t2 + t) * (x1 - x0) / (x1 - nodes[idx - 1]);
		weights[0] = -w0;
		weights[2] += w0;
	}
	else {
		float w0 = t3 - 2.0f * t2 + t;
		weights[0] = 0.0f;
		weights[1] -= w0;
		weights[2] += w0;
	}

	// Compute last node weight $w_3$
	if (idx + 2 < size) {
		float w3 = (t3 - t2) * (x1 - x0) / (nodes[idx + 2] - x0);
		weights[1] -= w3;
		weights[3] = w3;
	}
	else {
		float w3 = t3 - t2;
		weights[1] -= w3;
		weights[2] += w3;
		weights[3] = 0.0f;
	}

	return true;
}

inline float SampleCatmullRom(int n, const float* x, const float* f, const float* F, float u, float* fval, float* pdf) {
	// Map _u_ to a spline interval by inverting _F_
	u *= F[n - 1];
	int i = IntervalSearch(n, [&](int i) { return F[i] <= u; });

	// Look up $x_i$ and function values of spline segment _i_
	float x0 = x[i], x1 = x[i + 1];
	float f0 = f[i], f1 = f[i + 1];
	float width = x1 - x0;

	// Approximate derivatives using finite differences
	float d0, d1;
	if (i > 0) {
		d0 = width * (f1 - f[i - 1]) / (x1 - x[i - 1]);
	}
	else {
		d0 = f1 - f0;
	}
	if (i + 2 < n) {
		d1 = width * (f[i + 2] - f0) / (x[i + 2] - x0);
	}
	else {
		d1 = f1 - f0;
	}

	// Re-scale _u_ for continous spline sampling step
	u = (u - F[i]) / width;

	// Invert definite integral over spline segment and return solution

	// Set initial guess for $t$ by importance sampling a linear interpolant
	float t;
	if (f0 != f1) {
		t = (f0 - std::sqrt(std::max((float)0, f0 * f0 + 2.0f * u * (f1 - f0)))) / (f0 - f1);
	}
	else {
		t = u / f0;
	}

	float a = 0.0f, b = 1.0f, Fhat, fhat;
	while (true) {
		// Fall back to a bisection step when _t_ is out of bounds
		if (!(t > a && t < b)) {
			t = 0.5f * (a + b);
		}

		// Evaluate target function and its derivative in Horner form
		Fhat = t * (f0 +
			t * (0.5f * d0 +
				t * ((1.0f / 3.0f) * (-2.0f * d0 - d1) + f1 - f0 +
					t * (0.25f * (d0 + d1) + 0.5f * (f0 - f1)))));
		fhat = f0 +
			t * (d0 +
				t * (-2.0f * d0 - d1 + 3.0f * (f1 - f0) +
					t * (d0 + d1 + 2.0f * (f0 - f1))));

		// Stop the iteration if converged
		if (std::abs(Fhat - u) < 1e-6f || b - a < 1e-6f) {
			break;
		}

		// Update bisection bounds using updated _t_
		if (Fhat - u < 0.0f) {
			a = t;
		}
		else {
			b = t;
		}

		// Perform a Newton step
		t -= (Fhat - u) / fhat;
	}

	// Return the sample position and function value
	if (fval) {
		*fval = fhat;
	}
	if (pdf) {
		*pdf = fhat / F[n - 1];
	}

	return x0 + width * t;
}

inline float SampleCatmullRom2D(int size1, int size2, const float* nodes1,
	const float* nodes2, const float* values,
	const float* cdf, float alpha, float u, float* fval,
	float* pdf) {
	// Determine offset and coefficients for the _alpha_ parameter
	int offset;
	float weights[4];
	if (!CatmullRomWeights(size1, nodes1, alpha, &offset, weights)) {
		return 0.0f;
	}

	// Define a lambda function to interpolate table entries
	auto interpolate = [&](const float* array, int idx) {
		float value = 0.0f;
		for (int i = 0; i < 4; ++i)
			if (weights[i] != 0.0f) {
				value += array[(offset + i) * size2 + idx] * weights[i];
			}

		return value;
	};

	// Map _u_ to a spline interval by inverting the interpolated _cdf_
	float maximum = interpolate(cdf, size2 - 1);
	u *= maximum;
	int idx = IntervalSearch(size2, [&](int i) { return interpolate(cdf, i) <= u; });

	// Look up node positions and interpolated function values
	float f0 = interpolate(values, idx), f1 = interpolate(values, idx + 1);
	float x0 = nodes2[idx], x1 = nodes2[idx + 1];
	float width = x1 - x0;
	float d0, d1;

	// Re-scale _u_ using the interpolated _cdf_
	u = (u - interpolate(cdf, idx)) / width;

	// Approximate derivatives using finite differences of the interpolant
	if (idx > 0) {
		d0 = width * (f1 - interpolate(values, idx - 1)) /
			(x1 - nodes2[idx - 1]);
	}
	else {
		d0 = f1 - f0;
	}
	if (idx + 2 < size2) {
		d1 = width * (interpolate(values, idx + 2) - f0) /
			(nodes2[idx + 2] - x0);
	}
	else {
		d1 = f1 - f0;
	}

	// Invert definite integral over spline segment and return solution

	// Set initial guess for $t$ by importance sampling a linear interpolant
	float t;
	if (f0 != f1) {
		t = (f0 - std::sqrt(std::max((float)0, f0 * f0 + 2 * u * (f1 - f0)))) /
			(f0 - f1);
	}
	else {
		t = u / f0;
	}

	float a = 0.0f, b = 1.0f, Fhat, fhat;
	while (true) {
		// Fall back to a bisection step when _t_ is out of bounds
		if (!(t >= a && t <= b)) {
			t = 0.5f * (a + b);
		}

		// Evaluate target function and its derivative in Horner form
		Fhat = t * (f0 +
			t * (0.5f * d0 +
				t * ((1.0f / 3.0f) * (-2.0f * d0 - d1) + f1 - f0 +
					t * (0.25f * (d0 + d1) + 0.5f * (f0 - f1)))));
		fhat = f0 +
			t * (d0 +
				t * (-2.0f * d0 - d1 + 3.0f * (f1 - f0) +
					t * (d0 + d1 + 2.0f * (f0 - f1))));

		// Stop the iteration if converged
		if (std::abs(Fhat - u) < 1e-6f || b - a < 1e-6f) {
			break;
		}

		// Update bisection bounds using updated _t_
		if (Fhat - u < 0.0f) {
			a = t;
		}
		else {
			b = t;
		}

		// Perform a Newton step
		t -= (Fhat - u) / fhat;
	}

	// Return the sample position and function value
	if (fval) {
		*fval = fhat;
	}
	if (pdf) {
		*pdf = fhat / maximum;
	}

	return x0 + width * t;
}

inline float InvertCatmullRom(int n, const float* x, const float* values, float u) {
	if (!(u > values[0])) {
		return x[0];
	}
	else if (!(u < values[n - 1])) {
		return x[n - 1];
	}

	// Map _u_ to a spline interval by inverting _values_
	int i = IntervalSearch(n, [&](int i) { return values[i] <= u; });

	// Look up $x_i$ and function values of spline segment _i_
	float x0 = x[i], x1 = x[i + 1];
	float f0 = values[i], f1 = values[i + 1];
	float width = x1 - x0;

	// Approximate derivatives using finite differences
	float d0, d1;
	if (i > 0) {
		d0 = width * (f1 - values[i - 1]) / (x1 - x[i - 1]);
	}
	else {
		d0 = f1 - f0;
	}
	if (i + 2 < n) {
		d1 = width * (values[i + 2] - f0) / (x[i + 2] - x0);
	}
	else {
		d1 = f1 - f0;
	}

	// Invert the spline interpolant using Newton-Bisection
	float a = 0.0f, b = 1.0f, t = 0.5f;
	float Fhat, fhat;
	while (true) {
		// Fall back to a bisection step when _t_ is out of bounds
		if (!(t > a && t < b)) {
			t = 0.5f * (a + b);
		}

		// Compute powers of _t_
		float t2 = t * t, t3 = t2 * t;

		// Set _Fhat_ using Equation (8.27)
		Fhat = (2.0f * t3 - 3.0f * t2 + 1.0f) * f0 + (-2.0f * t3 + 3.0f * t2) * f1 +
			(t3 - 2.0f * t2 + t) * d0 + (t3 - t2) * d1;

		// Set _fhat_ using Equation (not present)
		fhat = (6.0f * t2 - 6.0f * t) * f0 + (-6.0f * t2 + 6.0f * t) * f1 +
			(3.0f * t2 - 4.0f * t + 1.0f) * d0 + (3.0f * t2 - 2.0f * t) * d1;

		// Stop the iteration if converged
		if (std::abs(Fhat - u) < 1e-6f || b - a < 1e-6f) {
			break;
		}

		// Update bisection bounds using updated _t_
		if (Fhat - u < 0) {
			a = t;
		}
		else {
			b = t;
		}

		// Perform a Newton step
		t -= (Fhat - u) / fhat;
	}

	return x0 + t * width;
}