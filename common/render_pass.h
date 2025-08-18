#pragma once

#include <shader.h>

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

// Forward declaration for friendship
class RenderGraph;

/**
 * @class RenderPass
 * @brief Abstract base class for render passes with named resource slots
 */
class RenderPass {
    friend class RenderGraph;  ///< Grant RenderGraph access to internal state

public:
    /**
     * @brief Construct a new RenderPass object
     * @param enabled Initial enabled state (true = active, false = disabled)
     */
    RenderPass(bool enabled = true) : enabled_(enabled), is_final_output_(false) {}

    /**
     * @brief Virtual destructor for proper cleanup of derived classes
     */
    virtual ~RenderPass() = default;

    /**
     * @brief Sets the enabled state of the render pass
     * @param enabled New enablement state (true = active, false = disabled)
     */
    void SetEnabled(bool enabled);

    /**
     * @brief Checks if the render pass is currently enabled
     * @return true if enabled and should execute, false otherwise
     */
    bool IsEnabled() const noexcept;

    /**
     * @brief Pure virtual function for executing rendering commands
     * @note Must be implemented by derived classes to perform actual rendering work
     */
    virtual void Execute() = 0;

    /**
     * @brief Declares a named input slot for resource dependencies
     * @param slot_name Name of the input slot
     */
    void DeclareInput(const std::string& slot_name);

    /**
     * @brief Declares a named output slot for produced resources
     * @param slot_name Name of the output slot
     * @param is_final Marks this output as the final render result (default = false)
     */
    void DeclareOutput(const std::string& slot_name, bool is_final = false);

protected:
    /// Input slot descriptor
    struct InputSlot {
        std::string name;     ///< Slot identifier
    };

    /// Output slot descriptor
    struct OutputSlot {
        std::string name;     ///< Slot identifier
    };

    bool enabled_;                         ///< Controls whether pass executes
    std::vector<InputSlot> input_slots_;   ///< Named input slots
    std::vector<OutputSlot> output_slots_; ///< Named output slots
    bool is_final_output_;                 ///< Marks pass as final output producer

    /// Populated by RenderGraph during compilation
    std::vector<TextureHandle> inputs_;   ///< Resolved input dependencies
    std::vector<TextureHandle> outputs_;  ///< Generated output resources
};

/**
 * @class SimplePass
 * @brief The simplest possible render pass for testing
 */
class SimplePass : public RenderPass {
    
public:
    SimplePass() {
        // Declare output slot
        DeclareOutput("output", true);  // Mark as final output
        outputs_.resize(1);
    }

    void Execute() override {
        // Create a fixed-size texture
        const int width = 1397;
        const int height = 721;

        // Generate checkered pattern
        std::vector<unsigned char> pixels(width * height * 4);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 4;
                bool checker = (x / 32 + y / 32) % 2 == 0;

                pixels[idx + 0] = checker ? 255 : 0;    // R
                pixels[idx + 1] = checker ? 0 : 255;    // G
                pixels[idx + 2] = 0;                    // B
                pixels[idx + 3] = 255;                  // A
            }
        }

        // Create texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Set output handle
        if (!outputs_.empty()) {
            outputs_[0].id = texture;
        }

        DEBUG("[debug] SimplePass executed");
    }

    GLuint GetId() {
        return outputs_[0].id;
    }
};

NAMESPACE_END(dream)