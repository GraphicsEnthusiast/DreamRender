#pragma once

#include "Utils.h"

struct AABB {
	Point3f pMin;
	Point3f pMax;
};

class DensityGrid {
public:
	virtual AABB GetBounds() = 0;

	virtual float GetDensity(const Point3f& pos) = 0;

	virtual float GetMaxDensity() = 0;
};

class OpenVDBGrid : public DensityGrid {
private:
	openvdb::FloatGrid::Ptr gridPtr;

public:
	OpenVDBGrid(const std::filesystem::path& filepath);

	virtual AABB GetBounds() override;

	virtual float GetDensity(const Point3f& pos) override;

	virtual float GetMaxDensity() override;
};