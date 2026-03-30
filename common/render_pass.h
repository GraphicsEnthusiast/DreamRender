#pragma once

#include <shader.h>
#include <scene.h>

NAMESPACE_BEGIN(dream)

/**
 * @struct TextureHandle
 * @brief Represents a handle to a GPU texture resource
 *
 * Uses integer-based identifier for lightweight texture referencing.
 * Provides equality comparison operator for container compatibility.
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
     * @brief Initializes the global SRGB to Spectrum conversion table TBO.
     */
    static void InitSRGBToSpectrumTable();

    /**
     * @brief Initializes the sobol matrices table TBO.
     */
    static void InitSobolMatricesTable();

    /**
     * @brief Gets the Sobol matrices TBO as a const reference
     * @return const reference to the Sobol matrices TBO
     * @note This function provides read-only access to the TBO
     */
    static const TBO& GetSobolMatricesTBO() noexcept;

    /**
     * @brief Gets the SRGB to Spectrum conversion table TBO as a const reference
     * @return const reference to the SRGB to Spectrum TBO
     * @note This function provides read-only access to the TBO
     */
    static const TBO& GetSRGBToSpectrumTBO() noexcept;

protected:
    InputSlotMap input_map_;             ///< Input slot name to resource mapping
    OutputSlotMap output_map_;           ///< Output slot name to resource mapping
    std::string name_;
    static unsigned int frame_counter_;

private:
    static std::unique_ptr<TBO> sobol_matrices_tbo_;
    static std::unique_ptr<TBO> srgb_to_spectrum_tbo_;
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