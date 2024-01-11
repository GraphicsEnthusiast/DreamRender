#include "DensityGrid.h"

OpenVDBGrid::OpenVDBGrid(const std::filesystem::path& filepath) {
	openvdb::initialize();

	// open vdb file
	openvdb::io::File file(filepath.generic_string());
	if (!file.open()) {
		printf("[OpenVDBGrid] failed to load {}", filepath.generic_string());

		assert(0);
	}

	// get density grid
	this->gridPtr = openvdb::gridPtrCast<openvdb::FloatGrid>(file.readGrid("density"));
	if (!this->gridPtr) {
		printf("[OpenVDBGrid] failed to load density grid");

		assert(0);
	}

	file.close();
}

MediumBox OpenVDBGrid::GetBounds() {
	MediumBox ret;
	const auto bbox = gridPtr->evalActiveVoxelBoundingBox();
	const auto pMin = gridPtr->indexToWorld(bbox.getStart());
	const auto pMax = gridPtr->indexToWorld(bbox.getEnd());
	for (int i = 0; i < 3; ++i) {
		ret.pMin[i] = pMin[i];
		ret.pMax[i] = pMax[i];
	}

	return ret;
}

float OpenVDBGrid::GetDensity(const Point3f& pos) {
	const auto gridSampler =
		openvdb::tools::GridSampler<openvdb::FloatGrid,
		openvdb::tools::BoxSampler>(*this->gridPtr);

	return gridSampler.wsSample(openvdb::Vec3f(pos[0], pos[1], pos[2]));
}

float OpenVDBGrid::GetMaxDensity() {
	float minValue, maxValue;
	gridPtr->evalMinMax(minValue, maxValue);

	return maxValue;
}