#pragma once

#include "Utils.h"

struct MediumBox {
	Point3f pMin;
	Point3f pMax;
};

class DensityGrid {
public:
	virtual MediumBox GetBounds() = 0;

	virtual float GetDensity(const Point3f& pos) = 0;

	virtual float GetMaxDensity() = 0;
};

class OpenVDBGrid : public DensityGrid {
private:
	openvdb::FloatGrid::Ptr gridPtr;

public:
	OpenVDBGrid(const std::filesystem::path& filepath);

	virtual MediumBox GetBounds() override;

	virtual float GetDensity(const Point3f& pos) override;

	virtual float GetMaxDensity() override;
};