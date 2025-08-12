#pragma once

#include <utils.h>

NAMESPACE_BEGIN(dream)

/**
 * @class Console
 * @brief Implements an interactive debugging console with command history and logging
 *
 * Features include:
 * - Command input with auto-completion
 * - Colored log output based on message type
 * - Persistent command history
 * - Filterable log display
 */
class Console {
public:
	Console();
	~Console();

	/// Log formatted message to console (supports printf-style formatting)
	void AddLog(const char* fmt, ...) IM_FMTARGS(2);

	/// Render console UI using ImGui
	void Draw();

protected:
	// String utility functions
	static int Stricmp(const char* s1, const char* s2);      ///< Case-insensitive string comparison
	static int Strnicmp(const char* s1, const char* s2, int n); ///< Case-insensitive comparison of first n characters
	static char* Strdup(const char* s);                     ///< Duplicate string with ImGui memory allocator
	static void Strtrim(char* s);                           ///< Remove trailing whitespace from string

	void ClearLog();                     ///< Clear all log entries
	void ExecCommand(const char* command_line); ///< Execute console command

	// Text input callback handlers
	static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);
	int TextEditCallback(ImGuiInputTextCallbackData* data);

protected:
	char input_buffer_[256];          ///< Command input buffer
	ImVector<char*> items_;            ///< Log items storage
	ImVector<const char*> commands_;   ///< Registered command list
	ImVector<char*> history_;          ///< Command history storage
	int history_pos_;                  ///< Current position in command history (-1 = new line)
	ImGuiTextFilter filter_;           ///< Log text filter
	bool auto_scroll_;                  ///< Automatic scroll to bottom when new log added
    bool scroll_to_bottom_;             ///< Request scroll to bottom on next frame
};

NAMESPACE_END(dream)