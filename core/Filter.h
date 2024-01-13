#pragma once

#include "Utils.h"

enum FilterType {
	BoxFilter,
	TentFilter,
	TriangleFilter,
	GaussianFilter
};

struct FilterParams {
	FilterType type;
};

class Filter {
public:
	Filter(FilterType type) : m_type(type) {}

	virtual Point2f FilterPoint2f(const Point2f& sample) = 0;

	inline FilterType GetType() const {
		return m_type;
	}

	static std::shared_ptr<Filter> Create(const FilterParams& params);

protected:
	FilterType m_type;
};

class Box : public Filter {
public:
	Box() : Filter(FilterType::BoxFilter) {}

	virtual Point2f FilterPoint2f(const Point2f& sample) override;
};

class Tent : public Filter {
public:
	Tent() : Filter(TentFilter) {}

	virtual Point2f FilterPoint2f(const Point2f& sample) override;
};

class Triangle : public Filter {
public:
	Triangle() : Filter(FilterType::TriangleFilter) {}

	virtual Point2f FilterPoint2f(const Point2f& sample) override;
};

class Gaussian : public Filter {
public:
	Gaussian() : Filter(FilterType::GaussianFilter) {}

	virtual Point2f FilterPoint2f(const Point2f& sample) override;
};