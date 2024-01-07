#include "Sampling.h"

AliasTable1D::AliasTable1D(const std::vector<float>& distrib) {
	std::queue<Element> greater, lesser;

	sumDistrib = 0.0f;
	for (auto i : distrib) {
		sumDistrib += i;
	}

	for (int i = 0; i < distrib.size(); i++) {
		float scaledPdf = distrib[i] * distrib.size();
		(scaledPdf >= sumDistrib ? greater : lesser).push(Element(i, scaledPdf));
	}

	table.resize(distrib.size(), Element(-1, 0.0f));

	while (!greater.empty() && !lesser.empty()) {
		auto [l, pl] = lesser.front();
		lesser.pop();
		auto [g, pg] = greater.front();
		greater.pop();

		table[l] = Element(g, pl);

		pg += pl - sumDistrib;
		(pg < sumDistrib ? lesser : greater).push(Element(g, pg));
	}

	while (!greater.empty()) {
		auto [g, pg] = greater.front();
		greater.pop();
		table[g] = Element(g, pg);
	}

	while (!lesser.empty()) {
		auto [l, pl] = lesser.front();
		lesser.pop();
		table[l] = Element(l, pl);
	}
}

int AliasTable1D::Sample(const Point2f& sample) {
	int rx = sample.x * table.size();
	if (rx == table.size()) {
		rx--;
	}
	float ry = sample.y;

	return (ry <= table[rx].second / sumDistrib) ? rx : table[rx].first;
}

AliasTable2D::AliasTable2D(const std::vector<float>& distrib, int width, int height) {
	std::vector<float> colDistrib(height);
	for (int i = 0; i < height; i++) {
		std::vector<float> table(distrib.begin() + i * width, distrib.begin() + (i + 1) * width);
		AliasTable1D rowDistrib(table);
		rowTables.push_back(rowDistrib);
		colDistrib[i] = rowDistrib.Sum();
	}
	colTable = AliasTable1D(colDistrib);
}

std::pair<int, int> AliasTable2D::Sample(const Point2f& sample1, const Point2f& sample2) {
	int row = colTable.Sample(sample1);

	return std::pair<int, int>(rowTables[row].Sample(sample2), row);
}