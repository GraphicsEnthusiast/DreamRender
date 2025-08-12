#include <console.h>

NAMESPACE_BEGIN(dream)

Console::Console() {
	ClearLog();
	memset(input_buffer_, 0, sizeof(input_buffer_));
	history_pos_ = -1;

	// "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
	commands_.push_back("HELP");
	commands_.push_back("HISTORY");
	commands_.push_back("CLEAR");
	auto_scroll_ = true;
	scroll_to_bottom_ = false;
	AddLog("Welcome to dream renderer!");
}

Console::~Console() {
	ClearLog();
	for (int i = 0; i < history_.Size; i++) {
		ImGui::MemFree(history_[i]);
	}
}

int Console::Stricmp(const char* s1, const char* s2) {
	int d;
	while ((d = std::toupper(*s2) - std::toupper(*s1)) == 0 && *s1) {
		s1++;
		s2++;
	}

	return d;
}

int Console::Strnicmp(const char* s1, const char* s2, int n) {
	int d = 0;
	while (n > 0 && (d = std::toupper(*s2) - std::toupper(*s1)) == 0 && *s1) {
		s1++;
		s2++;
		n--;
	}

	return d;
}

char* Console::Strdup(const char* s) {
	IM_ASSERT(s);
	size_t len = std::strlen(s) + 1;
	void* buf = ImGui::MemAlloc(len);
	IM_ASSERT(buf);

	return (char*)memcpy(buf, (const void*)s, len);
}

void Console::Strtrim(char* s) {
	char* str_end = s + std::strlen(s);
	while (str_end > s && str_end[-1] == ' ') {
		str_end--;
	}
	*str_end = 0;
}

void Console::ClearLog() {
	for (int i = 0; i < items_.Size; i++) {
		ImGui::MemFree(items_[i]);
	}
	items_.clear();
}

void Console::AddLog(const char* fmt, ...) IM_FMTARGS(2) {
	// FIXME-OPT
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
	buf[IM_ARRAYSIZE(buf) - 1] = 0;
	va_end(args);
	items_.push_back(Strdup(buf));
}

void Console::Draw() {
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Console", nullptr, ImGuiWindowFlags_None)) {
		ImGui::End();

		return;
	}

	// Command-line
	bool reclaim_focus = false;
	ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll
		| ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
	if (ImGui::InputText(
		"Command Input", input_buffer_, IM_ARRAYSIZE(input_buffer_), input_text_flags, &TextEditCallbackStub, (void*)this)) {
		char* s = input_buffer_;
		Strtrim(s);
		if (s[0]) {
			ExecCommand(s);
		}
		std::strcpy(s, "");
		reclaim_focus = true;
	}

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaim_focus) {
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
	}

	if (ImGui::SmallButton("Clear")) {
		ClearLog();
	}
	ImGui::SameLine();
	bool copy_to_clipboard = ImGui::SmallButton("Copy");
	// static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

	ImGui::Separator();

	// Options menu
	if (ImGui::BeginPopup("Options")) {
		ImGui::Checkbox("Auto Scroll", &auto_scroll_);
		ImGui::EndPopup();
	}

	// Options, Filter
	if (ImGui::Button("Options")) {
		ImGui::OpenPopup("Options");
	}
	ImGui::SameLine();
	filter_.Draw("Text Filter", 180);
	ImGui::Separator();

	// Reserve enough left-over height for 1 separator + 1 input text
	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_None,
		ImGuiWindowFlags_HorizontalScrollbar)) {
		if (ImGui::BeginPopupContextWindow()) {
			if (ImGui::Selectable("Clear")) {
				ClearLog();
			}
			ImGui::EndPopup();
		}

		// Display every line as a separate entry so we can change their color or add custom widgets.
		// If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
		// NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
		// to only process visible items_. The clipper will automatically measure the height of your first item and then
		// "seek" to display only items_ in the visible area.
		// To use the clipper we can replace your standard loop:
		//      for (int i = 0; i < items_.Size; i++)
		//   With:
		//      ImGuiListClipper clipper;
		//      clipper.Begin(items_.Size);
		//      while (clipper.Step())
		//         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		// - That your items_ are evenly spaced (same height)
		// - That you have cheap random access to your elements (you can access them given their index,
		//   without processing all the ones before)
		// You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
		// We would need random-access on the post-filtered list.
		// A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
		// or offsets of items_ that passed the filtering test, recomputing this array when user changes the filter,
		// and appending newly elements as they are inserted. This is left as a task to the user until we can manage
		// to improve this example code!
		// If your items_ are of variable height:
		// - Split them into same height items_ would be simpler and facilitate random-seeking into your list.
		// - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items_.
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		if (copy_to_clipboard) {
			ImGui::LogToClipboard();
		}
		for (const char* item : items_) {
			if (!filter_.PassFilter(item)) {
				continue;
			}

			// Normally you would store more information in your item than just a string.
			// (e.g. make items_[] an array of structure, store color/type etc.)
			ImVec4 color;
			bool has_color = false;
			if (strstr(item, "[error]")) {
				color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
				has_color = true;
			}
			else if (strstr(item, "[info]")) {
				color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
				has_color = true;
			}
			else if (strstr(item, "[debug]")) {
				color = ImVec4(0.4f, 0.4f, 1.0f, 1.0f);
				has_color = true;
			}
			else if (strstr(item, "[warning]")) {
				color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
				has_color = true;
			}
			if (has_color) {
				ImGui::PushStyleColor(ImGuiCol_Text, color);
			}
			ImGui::TextUnformatted(item);
			if (has_color) {
				ImGui::PopStyleColor();
			}
		}
		if (copy_to_clipboard) {
			ImGui::LogFinish();
		}

		// Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
		// Using a scrollbar or mouse-wheel will take away from the bottom edge.
		if (scroll_to_bottom_ || (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
			ImGui::SetScrollHereY(1.0f);
		}
		scroll_to_bottom_ = false;

		ImGui::PopStyleVar();
	}
	ImGui::EndChild();
	ImGui::Separator();
	ImGui::End();
}

void Console::ExecCommand(const char* command_line) {
	AddLog("# %s\n", command_line);

	// Insert into history_. First find match and delete it so it can be pushed to the back.
	// This isn't trying to be smart or optimal.
	history_pos_ = -1;
	for (int i = history_.Size - 1; i >= 0; i--) {
		if (Stricmp(history_[i], command_line) == 0) {
			ImGui::MemFree(history_[i]);
			history_.erase(history_.begin() + i);
			break;
		}
	}
	history_.push_back(Strdup(command_line));

	// Process command
	if (Stricmp(command_line, "CLEAR") == 0) {
		ClearLog();
	}
	else if (Stricmp(command_line, "HELP") == 0) {
		AddLog("Commands:");
		for (int i = 0; i < commands_.Size; i++) {
			AddLog("- %s", commands_[i]);
		}
	}
	else if (Stricmp(command_line, "HISTORY") == 0) {
		int first = history_.Size - 10;
		for (int i = first > 0 ? first : 0; i < history_.Size; i++) {
			AddLog("%3d: %s\n", i, history_[i]);
		}
	}
	else {
		AddLog("Unknown command: '%s'\n", command_line);
	}

	// On command input, we scroll to bottom even if auto_scroll_==false
	scroll_to_bottom_ = true;
}

int Console::TextEditCallbackStub(ImGuiInputTextCallbackData* data) {
	Console* console = (Console*)data->UserData;

	return console->TextEditCallback(data);
}

int Console::TextEditCallback(ImGuiInputTextCallbackData* data) {
	// AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
	switch (data->EventFlag) {
	case ImGuiInputTextFlags_CallbackCompletion: {
		// Example of TEXT COMPLETION

		// Locate beginning of current word
		const char* word_end = data->Buf + data->CursorPos;
		const char* word_start = word_end;
		while (word_start > data->Buf) {
			const char c = word_start[-1];
			if (c == ' ' || c == '\t' || c == ',' || c == ';') {
				break;
			}
			word_start--;
		}

		// Build a list of candidates
		ImVector<const char*> candidates;
		for (int i = 0; i < commands_.Size; i++) {
			if (Strnicmp(commands_[i], word_start, (int)(word_end - word_start)) == 0) {
				candidates.push_back(commands_[i]);
			}
		}

		if (candidates.Size == 0) {
			// No match
			AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
		}
		else if (candidates.Size == 1) {
			// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
			data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
			data->InsertChars(data->CursorPos, candidates[0]);
			data->InsertChars(data->CursorPos, " ");
		}
		else {
			// Multiple matches. Complete as much as we can..
			// So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
			int match_len = (int)(word_end - word_start);
			for (;; ) {
				int c = 0;
				bool all_candidates_matches = true;
				for (int i = 0; i < candidates.Size && all_candidates_matches; i++) {
					if (i == 0) {
						c = std::toupper(candidates[i][match_len]);
					}
					else if (c == 0 || c != std::toupper(candidates[i][match_len])) {
						all_candidates_matches = false;
					}
				}
				if (!all_candidates_matches) {
					break;
				}
				match_len++;
			}

			if (match_len > 0) {
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
			}

			// List matches
			AddLog("Possible matches:\n");
			for (int i = 0; i < candidates.Size; i++) {
				AddLog("- %s\n", candidates[i]);
			}
		}

		break;
	}
	case ImGuiInputTextFlags_CallbackHistory: {
	    	// Example of history_
	    	const int prev_history_pos = history_pos_;
	    	if (data->EventKey == ImGuiKey_UpArrow) {
	    		if (history_pos_ == -1) {
	    			history_pos_ = history_.Size - 1;
	    		}
	    		else if (history_pos_ > 0) {
	    			history_pos_--;
	    		}
	    	}
	    	else if (data->EventKey == ImGuiKey_DownArrow) {
	    		if (history_pos_ != -1) {
	    			if (++history_pos_ >= history_.Size) {
	    				history_pos_ = -1;
	    			}
	    		}
	    	}
	    
	    	// A better implementation would preserve the data on the current input line along with cursor position.
	    	if (prev_history_pos != history_pos_) {
	    		const char* history_str = (history_pos_ >= 0) ? history_[history_pos_] : "";
	    		data->DeleteChars(0, data->BufTextLen);
	    		data->InsertChars(0, history_str);
	    	}
	    }
	}

	return 0;
}

NAMESPACE_END(dream)