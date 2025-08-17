#include <renderer.h>
#include <interface.h>

NAMESPACE_BEGIN(dream)

Renderer::Renderer() : thread_pool_(1) {
	Initialize();
	thread_pool_.Enqueue([this] { RenderThreadMain(); });
}

Renderer::~Renderer() {
	stop_rendering_ = true;

	// Cleanup GL resources
	for (auto& buffer : buffers_) {
		if (buffer.fence) {
			glDeleteSync(buffer.fence);
		}
		if (buffer.texture) {
			glDeleteTextures(1, &buffer.texture);
		}
	}
}

void Renderer::SetRenderGraph(std::unique_ptr<RenderGraph> graph) {
	std::lock_guard lock(render_graph_mutex_);
	render_graph_ = std::move(graph);
}

GLuint Renderer::GetLatestTexture() const noexcept {
	// Check current buffer first
	if (buffers_[current_read_index_].ready.load()) {
		return buffers_[current_read_index_].texture;
	}

	// Find any available buffer
	for (unsigned int i = 1; i < BUFFER_COUNT; ++i) {
		unsigned int index = (current_read_index_ + i) % BUFFER_COUNT;
		if (buffers_[index].ready.load()) {
			current_read_index_ = index;
			return buffers_[index].texture;
		}
	}

	return 0; // No texture ready
}

void Renderer::RequestFrame() {
	frame_requested_.store(true);
}

void Renderer::Initialize() {
	// Create textures
	glGenTextures(BUFFER_COUNT, &buffers_[0].texture);

	for (unsigned int i = 0; i < BUFFER_COUNT; ++i) {
		glBindTexture(GL_TEXTURE_2D, buffers_[i].texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1,
			0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::RenderThreadMain() {
	while (!stop_rendering_.load()) {
		// Process frame request
		if (frame_requested_.exchange(false)) {
			std::unique_ptr<RenderGraph> current_graph;
			{
				std::lock_guard lock(render_graph_mutex_);
				if (render_graph_) {
					current_graph = std::move(render_graph_);
				}
			}

			if (current_graph) {
				// Execute render graph
				current_graph->Execute();

				// Update texture
				unsigned int write_index = current_write_index_;
				UpdateTexture(write_index);

				// Mark buffer ready
				buffers_[write_index].ready.store(true);

				// Create GPU fence
				buffers_[write_index].fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

				// Move to next buffer
				current_write_index_ = (write_index + 1) % BUFFER_COUNT;

				// Restore render graph
				{
					std::lock_guard lock(render_graph_mutex_);
					render_graph_ = std::move(current_graph);
				}
			}
		}
		else {
			// Avoid busy waiting
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

void Renderer::SetInterface(const std::weak_ptr<Interface>& interface) noexcept {
	interface_ = interface;
}

void Renderer::UpdateTexture(unsigned int buffer_index) {
	if (!render_graph_) {
		return;
	}

	TextureHandle output = render_graph_->GetFinalOutput();
	if (!output.IsValid()) {
		return;
	}

	auto shared_interface = interface_.lock();
	if (!shared_interface) {
		DEBUG("[warning] Interface instance expired during texture update.");

		return;
	}

	ImVec2 rendering_window_size = shared_interface->GetRenderingWindowSize();
	int width = static_cast<int>(rendering_window_size.x);
	int height = static_cast<int>(rendering_window_size.y);

	if (width <= 0 || height <= 0) {
		DEBUG("[warning] Invalid rendering window size: {}x{}.", width, height);

		return;
	}

	GLint current_width, current_height;
	glBindTexture(GL_TEXTURE_2D, buffers_[buffer_index].texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &current_width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &current_height);

	if (current_width != width || current_height != height) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height,
			0, GL_RGBA, GL_FLOAT, nullptr);
	}

	glCopyImageSubData(
		output.id, GL_TEXTURE_2D, 0, 0, 0, 0,
		buffers_[buffer_index].texture, GL_TEXTURE_2D, 0, 0, 0, 0,
		width, height, 1
	);
}

NAMESPACE_END(dream)