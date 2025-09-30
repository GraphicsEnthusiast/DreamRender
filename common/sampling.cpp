#include <sampling.h>

NAMESPACE_BEGIN(dream)

AliasTable1D::AliasTable1D(const std::vector<float>& distrib) {
    std::queue<Element> greater, lesser;

    sum_distrib = 0.0f;
    for (auto i : distrib) {
        sum_distrib += i;
    }

    for (unsigned int i = 0; i < distrib.size(); i++) {
        float scaled_pdf = distrib[i] * distrib.size();
        (scaled_pdf >= sum_distrib ? greater : lesser).push(Element(i, scaled_pdf));
    }

    table.resize(distrib.size(), Element(-1, 0.0f));

    while (!greater.empty() && !lesser.empty()) {
        auto [l, pl] = lesser.front();
        lesser.pop();
        auto [g, pg] = greater.front();
        greater.pop();

        table[l] = Element(g, pl);

        pg += pl - sum_distrib;
        (pg < sum_distrib ? lesser : greater).push(Element(g, pg));
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

    PrepareGPUData();
}

float AliasTable1D::Sum() const noexcept {
    return sum_distrib;
}

void AliasTable1D::PrepareGPUData() {
    gpu_data.resize(table.size());
    for (unsigned int i = 0; i < table.size(); i++) {
        gpu_data[i] = { table[i].first, table[i].second };
    }
}

const std::vector<AliasTableData>& AliasTable1D::GetGPUData() const noexcept {
    return gpu_data;
}

AliasTable2D::AliasTable2D(const std::vector<float>& distrib, unsigned int width, unsigned int height)
    : width(width), height(height) {
    row_tables.reserve(height);
    std::vector<float> colDistrib(height);

    for (unsigned int i = 0; i < height; i++) {
        std::vector<float> table(distrib.begin() + i * width, distrib.begin() + (i + 1) * width);
        AliasTable1D rowDistrib(table);
        row_tables.push_back(rowDistrib);
        colDistrib[i] = rowDistrib.Sum();
    }

    col_table = AliasTable1D(colDistrib);

    PrepareGPUData();
}

void AliasTable2D::PrepareGPUData() {
    row_tables_gpu_data.resize(height * width);
    for (unsigned int i = 0; i < height; i++) {
        const auto& rowData = row_tables[i].GetGPUData();
        std::copy(rowData.begin(), rowData.end(), row_tables_gpu_data.begin() + i * width);
    }

    col_tables_gpu_data = col_table.GetGPUData();
}

const std::vector<AliasTableData>& AliasTable2D::GetRowTablesGPUData() const noexcept {
    return row_tables_gpu_data;
}

const std::vector<AliasTableData>& AliasTable2D::GetColTableGPUData() const noexcept {
    return col_tables_gpu_data;
}

NAMESPACE_END(dream)