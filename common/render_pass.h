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
     * @brief Constructs a RenderPass with specified enable state
     */
    RenderPass() : enabled_(true) {}

    /**
     * @brief Virtual destructor for polymorphic deletion
     */
    virtual ~RenderPass() = default;

    /**
     * @brief Sets the activation state of the render pass
     * @param enabled New activation state
     */
    void SetEnabled(bool enabled);

    /**
     * @brief Checks current activation status
     * @return true if pass is enabled and should execute
     */
    bool IsEnabled() const noexcept;

    /**
     * @brief Pure virtual function for rendering command execution
     * @note Must be implemented by derived classes
     */
    virtual void Execute() = 0;

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
     * @return TextureHandle associated with the output slot (invalid if not found)
     */
    TextureHandle GetOutputTexture(const std::string& slot_name) const noexcept;

    /**
     * @brief Retrieves all input slot names and their associated texture handles.
     * @return const reference to the input slot map
     */
    const InputSlotMap& GetInputSlots() const noexcept;

    /**
     * @brief Retrieves all output slot names and their associated texture handles.
     * @return const reference to the output slot map
     */
    const OutputSlotMap& GetOutputSlots() const noexcept;

protected:
    bool enabled_;                       ///< Controls pass execution
    InputSlotMap input_map_;             ///< Input slot name to resource mapping
    OutputSlotMap output_map_;           ///< Output slot name to resource mapping
};

// Helper function to create an OpenGL texture
inline TextureHandle CreateColorTexture(int width, int height) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0,
		GL_RGBA, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return TextureHandle{ textureID };
}

/**
 * @class SimpleComputePass
 * @brief Demonstrates compute shader usage by generating a gradient texture
 */
class SimpleComputePass : public RenderPass {
public:
    SimpleComputePass() {
        // Create compute shader
        const char* compute_src = R"glsl(
            #version 460
            layout(local_size_x = 16, local_size_y = 16) in;
            layout(rgba32f, binding = 0) writeonly uniform image2D outputTex;
            
            // Uniform variable for time-based animation
            uniform float time;
            
            void main() {
                ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
                ivec2 size = imageSize(outputTex);
                vec2 uv = vec2(coord) / vec2(size);
                
                // Generate animated gradient pattern
                vec3 color = vec3(
                    sin(uv.x * 10.0 + time) * 0.5 + 0.5,
                    cos(uv.y * 10.0 + time) * 0.5 + 0.5,
                    uv.x * uv.y
                );
                
                imageStore(outputTex, coord, vec4(color, 1.0));
            }
        )glsl";

        shader_ = std::make_unique<ComputationShader>(compute_src);
    }

    void Execute() override {
		// Use compute shader
		shader_->Use();

        // Bind output texture
        glBindImageTexture(0, GetOutputTexture("Output").id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        // Set time uniform variable
        static float time = 0.0f;
        time += 0.01f;
        shader_->SetFloat("time", time);

        // Dispatch compute shader
        glDispatchCompute(512 / 16, 512 / 16, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
			GL_TEXTURE_FETCH_BARRIER_BIT);
    }

private:
    std::unique_ptr<ComputationShader> shader_;
};

/**
 * @class SimpleRasterPass
 * @brief Demonstrates rasterization shader by applying a color tint
 */
class SimpleRasterPass : public RenderPass {
public:
    SimpleRasterPass() {
        // Create shader
        const char* vertex_src = R"glsl(
            #version 460
            layout(location = 0) in vec3 position;
            layout(location = 1) in vec2 texCoord;
            out vec2 uv;
            void main() {
                gl_Position = vec4(position, 1.0);
                uv = texCoord;
            }
        )glsl";

        const char* fragment_src = R"glsl(
            #version 460
            in vec2 uv;
            out vec4 fragColor;
            uniform sampler2D inputTex;
            uniform float time;
            uniform vec3 tintColor = vec3(1.0, 0.7, 0.3); // Default tint color
            
            void main() {
                vec3 color = texture(inputTex, uv).rgb;
                
                // Apply pulsating tint effect
                float wave = sin(time * 2.0) * 0.3 + 0.7;
                vec3 tint = tintColor * wave;
                
                fragColor = vec4(color * tint, 1.0);
            }
        )glsl";

        shader_ = std::make_unique<RasterizationShader>(vertex_src, fragment_src);

        // Create fullscreen quad VAO
        CreateFullscreenQuad();
    }

    void Execute() override {
        // Get input texture
        auto input = GetInputTexture("Input");
        if (!input.IsValid()) return;

        // Use shader
        shader_->Use();

        // Set texture sampler uniform
        shader_->SetInt("inputTex", 0);

        // Set time uniform variable
        static float time = 0.0f;
        time += 0.01f;
        shader_->SetFloat("time", time);

        // Set tint color uniform
        glm::vec3 tintColor(0.8f, 0.5f, 0.2f);
        shader_->SetVector<3>("tintColor", tintColor);

        // Bind input texture to texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input.id);

        // Draw fullscreen quad
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
			GL_TEXTURE_FETCH_BARRIER_BIT);
    }

protected:
    void CreateFullscreenQuad() {
        // Fullscreen quad vertices
        float vertices[] = {
            // Position      // UV
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f
        };

        // Create VAO/VBO
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // UV attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    std::unique_ptr<RasterizationShader> shader_;
};

NAMESPACE_END(dream)