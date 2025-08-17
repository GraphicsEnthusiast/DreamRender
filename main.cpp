#include <interface.h>
#include <render_graph.h>
#include <render_pass.h>
#include <glad/glad.h>

NAMESPACE_BEGIN(dream)

/**
 * @class SimplePass
 * @brief The simplest possible render pass for testing
 */
    class SimplePass : public RenderPass {
    public:
        SimplePass() {
            // Declare output slot
            DeclareOutput("output", true);  // Mark as final output
        }

        void Execute() override {
            // Create a fixed-size texture
            const int width = 256;
            const int height = 256;

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
};

NAMESPACE_END(dream)

int main() {
	auto gui = dream::Interface::Create(1800, 1200);
	INFO("[info] Application started");

	auto graph = std::make_unique<dream::RenderGraph>();
	graph->AddPass("SimplePass", std::make_unique<dream::SimplePass>());

	gui->GetRenderer().SetRenderGraph(std::move(graph));
	gui->Render();
	return 0;
}