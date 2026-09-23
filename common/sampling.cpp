#include <sampling.h>

NAMESPACE_BEGIN(dream)

AliasTable1D::AliasTable1D(const std::vector<float>& distrib) {
    std::queue<Element> greater, lesser;

    max_table_ = 0.0f;
    for (auto i : distrib) {
        max_table_ += i;
    }

    for (unsigned int i = 0; i < distrib.size(); i++) {
        float scaled_pdf = distrib[i] * distrib.size();
        (scaled_pdf >= max_table_ ? greater : lesser).push(Element(i, scaled_pdf));
    }

    table_.resize(distrib.size(), Element(-1, 0.0f));

    while (!greater.empty() && !lesser.empty()) {
        auto [l, pl] = lesser.front();
        lesser.pop();
        auto [g, pg] = greater.front();
        greater.pop();

        table_[l] = Element(g, pl);

        pg += pl - max_table_;
        (pg < max_table_ ? lesser : greater).push(Element(g, pg));
    }

    while (!greater.empty()) {
        auto [g, pg] = greater.front();
        greater.pop();
        table_[g] = Element(g, pg);
    }

    while (!lesser.empty()) {
        auto [l, pl] = lesser.front();
        lesser.pop();
        table_[l] = Element(l, pl);
    }

    sum_table_ = 0.0f;
    for (const auto& e : table_) {
        sum_table_ += e.second;
    }

    PrepareGPUData();
}

float AliasTable1D::Max() const noexcept {
    return max_table_;
}

float AliasTable1D::Sum() const noexcept {
    return sum_table_;
}

void AliasTable1D::PrepareGPUData() {
    gpu_data_.resize(table_.size());
    for (unsigned int i = 0; i < table_.size(); i++) {
        gpu_data_[i] = { static_cast<float>(table_[i].first), table_[i].second };
    }
}

const std::vector<AliasTableData>& AliasTable1D::GetGPUData() const noexcept {
    return gpu_data_;
}

AliasTable2D::AliasTable2D(const std::vector<float>& distrib, unsigned int width, unsigned int height)
    : width_(width), height_(height) {
    row_table_.reserve(height);
    std::vector<float> col_distrib(height);

    for (unsigned int i = 0; i < height; i++) {
        std::vector<float> table(distrib.begin() + i * width, distrib.begin() + (i + 1) * width);
        AliasTable1D row_distrib(table);
        row_table_.push_back(row_distrib);
		col_distrib[i] = row_distrib.Max();
		row_maxs_.push_back(row_distrib.Max());
    }

    col_table_ = AliasTable1D(col_distrib);

    PrepareGPUData();
}

void AliasTable2D::PrepareGPUData() {
    row_table_gpu_data_.resize(height_ * width_);
    for (unsigned int i = 0; i < height_; i++) {
        const auto& row_data = row_table_[i].GetGPUData();
        std::copy(row_data.begin(), row_data.end(), row_table_gpu_data_.begin() + i * width_);
    }

    col_table_gpu_data_ = col_table_.GetGPUData();
}

const std::vector<AliasTableData>& AliasTable2D::GetRowTableGPUData() const noexcept {
    return row_table_gpu_data_;
}

const std::vector<AliasTableData>& AliasTable2D::GetColTableGPUData() const noexcept {
    return col_table_gpu_data_;
}

float AliasTable2D::Sum() const noexcept {
	return col_table_.Sum();
}

float AliasTable2D::Max() const noexcept {
	return col_table_.Max();
}

unsigned int AliasTable2D::GetWidth() const noexcept {
    return width_;
}

unsigned int AliasTable2D::GetHeight() const noexcept {
    return height_;
}

const std::vector<float>& AliasTable2D::GetRowMaxsGPUData() const noexcept {
	return row_maxs_;
}

NAMESPACE_END(dream)