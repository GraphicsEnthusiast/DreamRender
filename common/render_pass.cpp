#include <render_pass.h>

NAMESPACE_BEGIN(dream)

void RenderPass::SetEnabled(bool enabled) {
    enabled_ = enabled;
}

bool RenderPass::IsEnabled() const noexcept {
    return enabled_;
}

void RenderPass::DeclareInput(const std::string& slot_name) {
    input_slots_.emplace_back(InputSlot{ slot_name });
}

void RenderPass::DeclareOutput(const std::string& slot_name, bool is_final) {
    output_slots_.emplace_back(OutputSlot{ slot_name });
    if (is_final) {
        is_final_output_ = true;
    }
}

NAMESPACE_END(dream)