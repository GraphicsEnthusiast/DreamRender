#include <sampling.h>

NAMESPACE_BEGIN(dream)

AliasTable1D::AliasTable1D(const std::vector<float>& distrib) {
    std::queue<Element> greater, lesser;

    max_distrib = 0.0f;
    for (auto i : distrib) {
        max_distrib += i;
    }

    for (unsigned int i = 0; i < distrib.size(); i++) {
        float scaled_pdf = distrib[i] * distrib.size();
        (scaled_pdf >= max_distrib ? greater : lesser).push(Element(i, scaled_pdf));
    }

    table.resize(distrib.size(), Element(-1, 0.0f));

    while (!greater.empty() && !lesser.empty()) {
        auto [l, pl] = lesser.front();
        lesser.pop();
        auto [g, pg] = greater.front();
        greater.pop();

        table[l] = Element(g, pl);

        pg += pl - max_distrib;
        (pg < max_distrib ? lesser : greater).push(Element(g, pg));
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

    sum_distrib = 0.0f;
    for (const auto& e : table) {
        sum_distrib += e.second;
    }

    PrepareGPUData();
}

float AliasTable1D::Max() const noexcept {
    return max_distrib;
}

float AliasTable1D::Sum() const noexcept {
    return max_distrib;
}

void AliasTable1D::PrepareGPUData() {
    gpu_data.resize(table.size());
    for (unsigned int i = 0; i < table.size(); i++) {
        gpu_data[i] = { static_cast<float>(table[i].first), table[i].second };
    }
}

const std::vector<AliasTableData>& AliasTable1D::GetGPUData() const noexcept {
    return gpu_data;
}

AliasTable2D::AliasTable2D(const std::vector<float>& distrib, unsigned int width, unsigned int height)
    : width(width), height(height) {
    row_table.reserve(height);
    std::vector<float> colDistrib(height);

    for (unsigned int i = 0; i < height; i++) {
        std::vector<float> table(distrib.begin() + i * width, distrib.begin() + (i + 1) * width);
        AliasTable1D rowDistrib(table);
        row_table.push_back(rowDistrib);
        colDistrib[i] = rowDistrib.Sum();
    }

    col_table = AliasTable1D(colDistrib);

    PrepareGPUData();
}

void AliasTable2D::PrepareGPUData() {
    row_table_gpu_data.resize(height * width);
    for (unsigned int i = 0; i < height; i++) {
        const auto& row_data = row_table[i].GetGPUData();
        std::copy(row_data.begin(), row_data.end(), row_table_gpu_data.begin() + i * width);
    }

    col_table_gpu_data = col_table.GetGPUData();
}

const std::vector<AliasTableData>& AliasTable2D::GetRowtableGPUData() const noexcept {
    return row_table_gpu_data;
}

const std::vector<AliasTableData>& AliasTable2D::GetColTableGPUData() const noexcept {
    return col_table_gpu_data;
}

NAMESPACE_END(dream)