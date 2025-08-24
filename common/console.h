#pragma once

#include <utils.h>

NAMESPACE_BEGIN(dream)

/**
 * @class Console
 * @brief Implements an interactive debugging console with command history and logging
 */
class Console {
public:
    /**
     * @brief Constructs a Console object
     *
     * Initializes the console with default values:
     * - Clears input buffer
     * - Initializes history position
     * - Sets auto-scroll to true
     */
    Console();

    /**
     * @brief Destroys the Console object
     *
     * Cleans up all allocated resources:
     * - Frees log items
     * - Clears command history
     */
    ~Console();

    /**
     * @brief Logs a formatted message to the console
     * @param fmt Format string (printf-style)
     * @param ... Variable arguments matching the format string
     *
     * @note Uses IM_FMTARGS attribute for compile-time format checking
     */
    void AddLog(const char* fmt, ...) IM_FMTARGS(2);

    /**
     * @brief Renders the console UI using ImGui
     *
     * Draws the console window with:
     * - Log display area with filtering
     * - Command input field with history
     * - Auto-completion suggestions
     */
    void Draw();

protected:
    /**
     * @brief Case-insensitive string comparison
     * @param s1 First string to compare
     * @param s2 Second string to compare
     * @return Negative value if s1 < s2, 0 if equal, positive if s1 > s2
     */
    static int Stricmp(const char* s1, const char* s2);

    /**
     * @brief Case-insensitive comparison of first n characters
     * @param s1 First string to compare
     * @param s2 Second string to compare
     * @param n Number of characters to compare
     * @return Negative value if s1 < s2, 0 if equal, positive if s1 > s2
     */
    static int Strnicmp(const char* s1, const char* s2, int n);

    /**
     * @brief Duplicates a string using ImGui memory allocator
     * @param s String to duplicate
     * @return Pointer to duplicated string
     *
     * @note Caller is responsible for freeing the memory
     */
    static char* Strdup(const char* s);

    /**
     * @brief Removes trailing whitespace from a string
     * @param s String to trim (modified in-place)
     */
    static void Strtrim(char* s);

    /**
     * @brief Clears all log entries
     *
     * Frees all allocated log items and clears the log buffer
     */
    void ClearLog();

    /**
     * @brief Executes a console command
     * @param command_line Full command string to execute
     *
     * Processes the command and adds it to history
     */
    void ExecCommand(const char* command_line);

    /**
     * @brief Static stub for ImGui text edit callback
     * @param data ImGui input text callback data
     * @return 0 to continue, 1 to prevent default handling
     */
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);

    /**
     * @brief Handles text input callbacks
     * @param data ImGui input text callback data
     * @return 0 to continue, 1 to prevent default handling
     *
     * Implements:
     * - Tab completion
     * - Command history navigation
     */
    int TextEditCallback(ImGuiInputTextCallbackData* data);

protected:
    char input_buffer_[256];          ///< Buffer for command input (max 255 characters + null terminator)
    ImVector<char*> items_;           ///< Storage for log items (dynamically allocated strings)
    ImVector<const char*> commands_;  ///< List of registered commands (static strings)
    ImVector<char*> history_;         ///< Command history storage (dynamically allocated strings)
    int history_pos_;                 ///< Current position in command history (-1 = new command)
    ImGuiTextFilter filter_;          ///< Text filter for log display
    bool auto_scroll_;                ///< Automatically scroll to bottom when new log added
    bool scroll_to_bottom_;           ///< Flag to request scroll to bottom on next frame
    std::mutex log_mutex_;            ///< Thread synchronization primitive for log operations
};

NAMESPACE_END(dream)