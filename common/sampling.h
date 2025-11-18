#pragma once

#include <utils.h>

NAMESPACE_BEGIN(dream)

/**
 * @struct AliasTable1DData
 * @brief Data structure for 1D alias table elements used for GPU transfer
 */
struct AliasTableData {
    float alias; ///< Alias index for this table entry
    float prob;  ///< Probability value for this table entry
};

/**
 * @class AliasTable1D
 * @brief Implements 1D alias method for efficient sampling from discrete distributions
 */
class AliasTable1D {
public:
    typedef std::pair<int, float> Element;  ///< Internal element type

    /**
     * @brief Default constructor
     */
    AliasTable1D() = default;

    /**
     * @brief Constructs a 1D alias table from a probability distribution
     * @param distrib Probability distribution to build the table from
     */
    AliasTable1D(const std::vector<float>& distrib);

    /**
     * @brief Gets the max of the probability distribution
     * @return Max of all probabilities in the distribution
     */
    float Max() const noexcept;

    /**
     * @brief Gets the sum of the probability distribution
     * @return Sum of all probabilities in the distribution
     */
    float Sum() const noexcept;

    /**
     * @brief Gets the GPU-ready data
     * @return Const reference to GPU data vector
     */
    const std::vector<AliasTableData>& GetGPUData() const noexcept;

protected:
    /**
     * @brief Prepares GPU-friendly data from the internal table
     */
    void PrepareGPUData();

protected:
    std::vector<Element> table;             ///< Internal alias table
    std::vector<AliasTableData> gpu_data;   ///< GPU-ready data
    float max_table;                        ///< Max of the table
    float sum_table;                        ///< Sum of the table
};

/**
 * @class AliasTable2D
 * @brief Implements 2D alias method using multiple 1D alias table
 */
class AliasTable2D {
public:
    /**
     * @brief Default constructor
     */
    AliasTable2D() = default;

    /**
     * @brief Constructs a 2D alias table from a 2D probability distribution
     * @param distrib Flat array representing 2D probability distribution
     * @param width Width of the 2D distribution
     * @param height Height of the 2D distribution
     */
    AliasTable2D(const std::vector<float>& distrib, unsigned int width, unsigned int height);

    /**
     * @brief Gets the GPU-ready data for row table
     * @return Const reference to row table GPU data
     */
    const std::vector<AliasTableData>& GetRowtableGPUData() const noexcept;

    /**
     * @brief Gets the GPU-ready data for column table
     * @return Const reference to column table GPU data
     */
    const std::vector<AliasTableData>& GetColTableGPUData() const noexcept;

protected:
    /**
     * @brief Prepares GPU-friendly data from the internal table
     */
    void PrepareGPUData();

protected:
    std::vector<AliasTable1D> row_table;              ///< Row alias table
    AliasTable1D col_table;                            ///< Column alias table
    std::vector<AliasTableData> row_table_gpu_data;   ///< GPU data for row table
    std::vector<AliasTableData> col_table_gpu_data;   ///< GPU data for column table
    unsigned int width;                                ///< Width of 2D distribution
    unsigned int height;                               ///< Height of 2D distribution
};


NAMESPACE_END(dream)