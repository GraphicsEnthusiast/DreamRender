#pragma once

#define TINYOBJLOADER_IMPLEMENTATION
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#if !defined(NAMESPACE_BEGIN)
#define NAMESPACE_BEGIN(name) namespace name {
#endif

#if !defined(NAMESPACE_END)
#define NAMESPACE_END(name) }
#endif

// Use default logger for now...
#define DEBUG(...) SPDLOG_LOGGER_DEBUG(spdlog::default_logger(), __VA_ARGS__)
#define INFO(...)  SPDLOG_LOGGER_INFO(spdlog::default_logger(), __VA_ARGS__)
#define WARN(...)  SPDLOG_LOGGER_WARN(spdlog::default_logger(), __VA_ARGS__)
#define ERROR(...) SPDLOG_LOGGER_ERROR(spdlog::default_logger(), __VA_ARGS__)

#include <iostream>
#include <cctype>
#include <functional>
#include <numeric>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <map>
#include <vector>
#include <unordered_map>
#include <queue>
#include <array>
#include <list>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/callback_sink.h>
#include <fmt/core.h>

NAMESPACE_BEGIN(dream)

using Vector2u = glm::uvec2;
using Vector2i = glm::ivec2;
using Vector2f = glm::vec2;
using Vector3u = glm::uvec3;
using Vector3i = glm::ivec3;
using Vector3f = glm::vec3;
using Vector4u = glm::uvec4;
using Vector4i = glm::ivec4;
using Vector4f = glm::vec4;
using Matrix3f = glm::mat3x3;
using Matrix4f = glm::mat4x4;

using Point2f = Vector2f;
using Point3f = Vector3f;
using Point4f = Vector4f;
using Point2i = Vector2i;
using Point3i = Vector3i;
using Point4i = Vector4i;
using Point2u = Vector2u;
using Point3u = Vector3u;
using Point4u = Vector4u;

constexpr float MaxFloat = FLT_MAX;
constexpr float MinFloat = -MaxFloat;
constexpr float InfFloat = std::numeric_limits<float>::infinity();

NAMESPACE_END(dream)