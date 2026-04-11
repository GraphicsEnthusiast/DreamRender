#pragma once

#include <shader.h>
#include <scene.h>

NAMESPACE_BEGIN(dream)

/**
 * @struct TextureHandle
 * @brief Represents a handle to a GPU texture resource
 */
struct TextureHandle {
    GLuint id = UINT32_MAX; ///< Unique texture identifier (UINT32_MAX = invalid handle)

    /**
     * @brief Equality comparison operator
     * @param other TextureHandle to compare against
     * @return true if texture IDs match
     */
    inline bool operator==(const TextureHandle& other) const {
        return other.id == id;
    }

    /**
     * @brief Checks if valid
     * @return true if valid
     */
    inline bool IsValid() const noexcept {
        return UINT32_MAX != id;
    }
};

/**
 * @struct ResourceEdge
 * @brief Defines explicit connection between render passes
 */
struct ResourceEdge {
    std::string src_pass;     ///< Source pass name
    std::string src_output;   ///< Source output slot name
    std::string dst_pass;     ///< Destination pass name
    std::string dst_input;    ///< Destination input slot name
};

/**
 * @class RenderPass
 * @brief Abstract base class for render passes with named resource slots
 */
class RenderPass {
public:
    /// Map type for input slot name to texture handle
    using InputSlotMap = std::unordered_map<std::string, TextureHandle>;

    /// Map type for output slot name to texture handle
    using OutputSlotMap = std::unordered_map<std::string, TextureHandle>;

    /**
     * @brief Constructor for polymorphic deletion
     */
    RenderPass() = default;

    /**
     * @brief Virtual destructor for polymorphic deletion
     */
    virtual ~RenderPass() = default;

    /**
     * @brief Pure virtual function for rendering command execution
     * @note Must be implemented by derived classes
     */
    virtual void Execute() = 0;

    /**
     * @brief Get the current frame counter value
     */
    static unsigned int GetFrameCounter() noexcept;

    /**
     * @brief Increase the current frame counter value
     */
    static void IncreaseFrameCounter() noexcept;

    /**
     * @brief Sets the texture handle for a specified input slot.
     * @param slot_name Name identifier of the input slot to modify.
     * @param handle Texture handle to assign to the slot.
     */
    void SetInputTexture(const std::string& slot_name, const TextureHandle& handle);

    /**
     * @brief Sets the texture handle for a specified output slot.
     * @param slot_name Name identifier of the output slot to modify.
     * @param handle Texture handle to assign to the slot.
     */
    void SetOutputTexture(const std::string& slot_name, const TextureHandle& handle);

    /**
     * @brief Retrieves the texture handle for a specified input slot.
     * @param slot_name Name identifier of the input slot to retrieve.
     * @return TextureHandle associated with the input slot (invalid if not found)
     */
    TextureHandle GetInputTexture(const std::string& slot_name) const noexcept;

    /**
     * @brief Retrieves the texture handle for a specified output slot.
     * @param slot_name Name identifier of the output slot to retrieve.
     * @return TextureHandle associated with the output slot (invalid if not found).
     */
    TextureHandle GetOutputTexture(const std::string& slot_name) const noexcept;

    /**
     * @brief Gets the current name of the object.
     * @return Current name of the object (guaranteed valid due to noexcept).
     */
    const std::string& GetName() const noexcept;

    /**
     * @brief Initializes the global SRGB to Spectrum conversion table SSBO.
     */
    static void InitSRGBToSpectrumTable();

    /**
     * @brief Initializes the Sobol matrices table SSBO.
     */
    static void InitSobolMatricesTable();

    /**
     * @brief Initializes the CIE data table SSBO.
     */
    static void InitCIETable();

    /**
     * @brief Binds the SRGB to Spectrum SSBO to a specific binding point
     * @param index The binding point index to bind to
     */
    static void BindSRGBToSpectrumSSBO(GLuint index) noexcept;

    /**
     * @brief Binds the Sobol matrices SSBO to a specific binding point
     * @param index The binding point index to bind to
     */
    static void BindSobolMatricesSSBO(GLuint index) noexcept;

    /**
     * @brief Binds the CIE data SSBO to a specific binding point
     * @param index The binding point index to bind to
     */
    static void BindCIESSBO(GLuint index) noexcept;

    /**
     * @brief Gets the SRGB to Spectrum conversion table SSBO
     * @return Reference to the SRGB to Spectrum SSBO
     * @note This function provides direct access to the SSBO
     */
    static SSBO& GetSRGBToSpectrumSSBO() noexcept;

    /**
     * @brief Gets the Sobol matrices table SSBO
     * @return Reference to the Sobol matrices SSBO
     * @note This function provides direct access to the SSBO
     */
    static SSBO& GetSobolMatricesSSBO() noexcept;

    /**
     * @brief Gets the CIE data table SSBO
     * @return Reference to the CIE data SSBO
     * @note This function provides direct access to the SSBO
     */
    static SSBO& GetCIESSBO() noexcept;

protected:
    InputSlotMap input_map_;             ///< Input slot name to resource mapping
    OutputSlotMap output_map_;           ///< Output slot name to resource mapping
    std::string name_;
    static unsigned int frame_counter_;
    static std::unique_ptr<SSBO> sobol_matrices_ssbo_;      ///< SSBO for Sobol matrices
    static std::unique_ptr<SSBO> srgb_to_spectrum_ssbo_;    ///< SSBO for SRGB to Spectrum conversion table
    static std::unique_ptr<SSBO> cie_ssbo_;                 ///< SSBO for CIE data tables
};

/**
 * @class ProgressivePass
 * @brief Blends current frame with previous frame for progressive refinement
 */
class ProgressivePass : public RenderPass {
public:
    /**
     * @brief Constructor for progressive rendering pass
     * @param width Render target width
     * @param height Render target height
     */
    ProgressivePass(unsigned int width, unsigned int height);

    /**
     * @brief Executes the progressive blending computation
     */
    void Execute() override;

protected:
    std::unique_ptr<ComputationShader> shader_; ///< Compute shader for blending
    unsigned int width_;                        ///< Render target width
    unsigned int height_;                       ///< Render target height
};

/**
 * @class PostProcessingPass
 * @brief Applies tone mapping (ACES) and gamma correction to the input image
 */
class PostProcessingPass : public RenderPass {
public:
    /**
     * @brief Constructor for post-processing pass
     * @param width Render target width
     * @param height Render target height
     */
    PostProcessingPass(unsigned int width, unsigned int height);

    /**
     * @brief Executes the post-processing operations
     * @note Applies ACES tone mapping and gamma correction
     */
    void Execute() override;

protected:
    std::unique_ptr<ComputationShader> shader_;  ///< Compute shader for post-processing
    unsigned int width_;                         ///< Render target width
    unsigned int height_;                        ///< Render target height
};

/**
 * @class SimpleComputePass
 * @brief Demonstrates compute shader usage by generating a gradient texture.
 */
class SimpleComputePass : public RenderPass {
public:
    /**
     * @brief Constructor for the simple compute pass.
     * @param width Width of the output texture and computation domain.
     * @param height Height of the output texture and computation domain.
     */
    SimpleComputePass(unsigned int width, unsigned int height);

    /**
     * @brief Executes the compute shader dispatch.
     * @note Overrides the pure virtual function from RenderPass.
     */
    void Execute() override;

protected:
    std::unique_ptr<ComputationShader> shader_; ///< Compute shader instance
    unsigned int width_;                        ///< Width of the render target
    unsigned int height_;                       ///< Height of the render target
};

NAMESPACE_END(dream)