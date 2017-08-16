// Mini memory editor for ImGui (to embed in your game/tools)
// v0.10
// Animated gif: https://cloud.githubusercontent.com/assets/8225057/9028162/3047ef88-392c-11e5-8270-a54f8354b208.gif
//
// You can adjust the keyboard repeat delay/rate in ImGuiIO.
// The code assume a mono-space font for simplicity! If you don't use the default font, use ImGui::PushFont()/PopFont() to switch to a mono-space font before caling this.
//
// Usage:
//   static MemoryEditor memory_editor;                                                     // save your state somewhere
//   memory_editor.Draw("Memory Editor", mem_block, mem_block_size, (size_t)mem_block);     // run
//
// TODO: better resizing policy (ImGui doesn't have flexible window resizing constraints yet)

#include <imgui.h>
#include <stdint.h>
#include <atlcoll.h>
#include "imgui_tablabels.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BGDM_EXCEPTION(msg) ExceptHandler(msg, GetExceptionCode(), GetExceptionInformation(), __FILE__, __FUNCTION__, __LINE__)
DWORD ExceptHandler(const char *msg, DWORD code, EXCEPTION_POINTERS *ep, const char *file, const char *func, int line);

#ifdef __cplusplus
}
#endif

struct MemoryEditor
{
    bool				IsOpen;
    bool				AllowEdits;
    int64_t				DataEditingAddr;
    bool				DataEditingTakeFocus;
    char				DataInput[32];
    char				AddrInput[32];
	char				OffsetInput[32];
	char				BaseAddrInput[32];

private:

	typedef struct TabData
	{
		int					mode;
		int					bytes_per_line;
		ATL::CAtlStringA	name;
		uintptr_t			base_addr;
		uintptr_t			edit_addr;
		unsigned char*		mem_data;
		int					mem_size;
	} TabData;

	ATL::CAtlStringA					m_title;
	bool								m_is64;
	int									m_tabCount;
	int									m_tabSelected;
	ATL::CAtlArray<int>					m_tabOrder;
	ATL::CAtlArray<const char *>		m_tabNames;
	ATL::CAtlArray<TabData>				m_tabData;

public:

    MemoryEditor()
    {
		Init();
    }

	MemoryEditor(const char *title, const char *tabTitle = NULL, int tabCount = 4, int selectedTab = 0,
		unsigned char* mem_data = NULL, int mem_size = 0x1000, uintptr_t base_display_addr = 0)
	{
		Init(title, tabTitle, tabCount, selectedTab, mem_data, mem_size, base_display_addr);
	}

	void Init(const char *title = NULL, const char *tabTitle = NULL, int tabCount = 4, int selectedTab = 0,
		unsigned char* mem_data = NULL, int mem_size = 0x1000, uintptr_t base_display_addr = 0)
	{
		IsOpen = false;
		AllowEdits = true;
		DataEditingAddr = -1;
		DataEditingTakeFocus = false;
		strcpy(DataInput, "");
		strcpy(AddrInput, "");

		m_title = title ? title : "Memory Editor";
		m_is64 = (sizeof(uintptr_t) == 8);

		ImGuiIO& io = ImGui::GetIO();
		io.KeyRepeatDelay = 0.200f;
		io.KeyRepeatRate = 0.010f;

		InitTabs(tabTitle ? tabTitle : "Mem", tabCount, selectedTab);

		SetBaseAddr(mem_data, mem_size, base_display_addr, selectedTab);
	}

	void InitTabs(const char *tabTitle, int tabCount, int tabSelected)
	{
		if (tabCount <= 0) tabCount = 1;
		if (tabSelected > tabCount - 1) tabSelected = 0;

		m_tabCount = tabCount;
		m_tabSelected = tabSelected;

		m_tabData.SetCount(tabCount);
		m_tabOrder.SetCount(tabCount);
		m_tabNames.SetCount(tabCount);

		for (int i = 0; i < tabCount; i++) {
			m_tabOrder[i] = i;
			m_tabData[i].bytes_per_line = 16;
			m_tabData[i].name.Format("%s %d", tabTitle, i+1);
			m_tabNames[i] = m_tabData[i].name.GetString();
		}
	}

	unsigned char *GetBaseAddr(int selected_tab = -1)
	{
		if (selected_tab < 0) selected_tab = m_tabSelected;
		else if (selected_tab > m_tabCount - 1) selected_tab = 0;

		return m_tabData[selected_tab].mem_data;
	}

	void SetBaseAddr(unsigned char* mem_data, int mem_size = 0x1000, uintptr_t base_display_addr = 0, int selected_tab = -1)
	{
		if (selected_tab < 0) selected_tab = m_tabSelected;
		else if (selected_tab > m_tabCount - 1) selected_tab = 0;

		m_tabData[selected_tab].mem_data = mem_data;
		m_tabData[selected_tab].mem_size = mem_data ? mem_size : 0;
		m_tabData[selected_tab].base_addr = base_display_addr ? base_display_addr : (uintptr_t)mem_data;
		m_tabData[selected_tab].edit_addr = 0;
	}

	void Open(unsigned char* mem_data, int mem_size = 0x1000, size_t base_display_addr = 0, int selected_tab = -1)
	{
		IsOpen = true;
		if (selected_tab >= 0) {
			m_tabSelected = selected_tab;
			if (m_tabSelected > m_tabCount - 1) m_tabSelected = 0;
		}
		SetBaseAddr(mem_data, mem_size, base_display_addr, m_tabSelected);
		Draw();
	}

	void FindOffset(size_t offset)
	{
		ImGui::BeginChild("##scrolling");
		ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + (offset / m_tabData[m_tabSelected].bytes_per_line) * ImGui::GetTextLineHeight());
		ImGui::EndChild();
	}

	void FindStrOffset(const char *str, bool isAddr)
	{
		size_t goto_addr = 0;
		if (sscanf(str, "%I64X", &goto_addr) == 1)
		{
			if (isAddr) goto_addr -= m_tabData[m_tabSelected].base_addr;
			if (goto_addr >= 0 && goto_addr < m_tabData[m_tabSelected].mem_size)
			{
				FindOffset(goto_addr);
				DataEditingAddr = goto_addr;
				DataEditingTakeFocus = true;
				m_tabData[m_tabSelected].edit_addr = m_tabData[m_tabSelected].base_addr + goto_addr;
			}
		}
	}

	void Draw()
    {
        if (ImGui::Begin(m_title, &IsOpen))
        {
			const bool tabChanged = ImGui::TabLabels(m_tabNames.GetData(), m_tabCount, m_tabSelected, m_tabOrder.GetData());

			TabData* tabData = &m_tabData[m_tabSelected];

			// no. of bytes per row for
			// 8/16/32/64 bit modes
			// 1/2/4/8 bytes respectively
			int bytes_per_row = 1;
			for (int i = 0; i < tabData->mode; i++)
				bytes_per_row *= 2;
			int number_of_rows = tabData->bytes_per_line / bytes_per_row;

			bool data_next = false;
			int addr_digits_count = sizeof(uintptr_t) * 2;
			float glyph_width = ImGui::CalcTextSize("F").x;
			float line_height = ImGui::GetTextLineHeight();

			float input_width = 0;
			switch (tabData->mode) {
			case(0):
				input_width = ImGui::CalcTextSize("FF").x;
				break;
			case(1):
				input_width = ImGui::CalcTextSize("FFFF").x;
				break;
			case(2):
				input_width = ImGui::CalcTextSize("FFFFFFFF").x;
				break;
			case(3):
				input_width = ImGui::CalcTextSize("FFFFFFFFFFFFFFFF").x;
				break;
			};
			float cell_width = glyph_width + input_width; // "FF " we include trailing space in the width to easily catch clicks everywhere

			{
				ImGui::Separator();
				ImGui::AlignFirstTextHeightToWidgets();
				ImGui::PushAllowKeyboardFocus(false);

				ATL::CAtlStringA str;
				static float spacing = 130.0f;

				str.Format("%0*I64X", addr_digits_count, tabData->base_addr);
				memset(BaseAddrInput, 0, sizeof(BaseAddrInput));
				strncpy(BaseAddrInput, str.GetString(), sizeof(BaseAddrInput));
				ImGui::Text("Base Address:"); ImGui::SameLine(spacing);
				ImGui::PushItemWidth(140);
				if (ImGui::InputText("##baseaddr", BaseAddrInput, sizeof(BaseAddrInput), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue))
				{
					size_t base_addr;
					if (sscanf(BaseAddrInput, "%I64X", &base_addr) == 1)
					{
						SetBaseAddr((unsigned char *)base_addr);
					}
				}

				bool scroll = false;
				str.Format("%0*I64X", addr_digits_count, tabData->edit_addr ? tabData->edit_addr : tabData->base_addr);
				memset(AddrInput, 0, sizeof(AddrInput));
				strncpy(AddrInput, str.GetString(), sizeof(AddrInput));
				ImGui::Text("Edit Address:"); ImGui::SameLine(spacing);
				scroll |= ImGui::InputText("##curraddr", AddrInput, sizeof(AddrInput), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::SameLine();
				scroll |= ImGui::Button("Find Address", ImVec2(100, 0));
				if (scroll) FindStrOffset(AddrInput, true);

				scroll = false;
				int64_t goto_addr = 0;
				str.Empty();
				if (sscanf(AddrInput, "%I64X", &goto_addr) == 1) {
					if (goto_addr == 0) str.Format("0");
					else if (goto_addr - (int64_t)tabData->base_addr < 0)
						str.Format("-%I64X", tabData->base_addr-goto_addr);
					else str.Format("%I64X", goto_addr-tabData->base_addr);
					memset(OffsetInput, 0, sizeof(OffsetInput));
					strncpy(OffsetInput, str.GetString(), sizeof(OffsetInput));
				}
				ImGui::Text("Edit Offset:"); ImGui::SameLine(spacing);
				scroll |= ImGui::InputText("##offset", OffsetInput, sizeof(OffsetInput), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::SameLine();
				scroll |= ImGui::Button("Find Offset", ImVec2(100, 0));
				if (scroll) FindStrOffset(OffsetInput, false);

				ImGui::PopItemWidth();
				ImGui::PopAllowKeyboardFocus();
				ImGui::Separator();
			}

			// TODO:
			// Change the static inside TabLabels so we can drag/drop when there are 
			// multiple tabs in one window 
			// static int draggingTabIndex
			static const char* modeNames[] = { "8-bit","16-bit","32-bit","64-bit" };
			static int modeOrder[] = { 0,1,2,3 };
			const bool modeChanged = ImGui::TabLabels(modeNames, sizeof(modeNames) / sizeof(modeNames[0]), tabData->mode, modeOrder);

			// Verify we can access the memory block
			__try {
				if (tabData->mem_size > 0) {
					unsigned char access = tabData->mem_data[0];
					if (access) {}
				}
			}
			__except (BGDM_EXCEPTION("[MemoryEditor::Draw] access violation #1")) {

				// Set our size to 0 to skip memory access
				tabData->mem_size = 0;
			}

			if (!tabData->mem_data || tabData->mem_size == 0) goto end;

            ImGui::BeginChild("##scrolling", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

			bool needPopID = false;
			__try {

				int bytes_per_line = tabData->bytes_per_line;
				int line_total_count = (int)((tabData->mem_size + bytes_per_line - 1) / bytes_per_line);
				ImGuiListClipper clipper(line_total_count, line_height);
				int visible_start_addr = clipper.DisplayStart * bytes_per_line;
				int visible_end_addr = clipper.DisplayEnd * bytes_per_line;

				if (!AllowEdits || DataEditingAddr >= tabData->mem_size)
					DataEditingAddr = -1;

				int64_t data_editing_addr_backup = DataEditingAddr;
				if (DataEditingAddr != -1)
				{
					if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)) && DataEditingAddr >= number_of_rows)								{ DataEditingAddr -= bytes_per_line; DataEditingTakeFocus = true; }
					else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)) && DataEditingAddr < tabData->mem_size - number_of_rows)	{ DataEditingAddr += bytes_per_line; DataEditingTakeFocus = true; }
					else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)) && DataEditingAddr > 0)									{ DataEditingAddr -= bytes_per_row; DataEditingTakeFocus = true; }
					else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)) && DataEditingAddr < tabData->mem_size - 1)				{ DataEditingAddr += bytes_per_row; DataEditingTakeFocus = true; }
				}
				if ((DataEditingAddr / number_of_rows) != (data_editing_addr_backup / number_of_rows))
				{
					// Track cursor movements
					float scroll_offset = ((DataEditingAddr / number_of_rows) - (data_editing_addr_backup / number_of_rows)) * line_height;
					bool scroll_desired = (scroll_offset < 0.0f && DataEditingAddr < visible_start_addr + number_of_rows *2) || (scroll_offset > 0.0f && DataEditingAddr > visible_end_addr - number_of_rows *2);
					if (scroll_desired)
						ImGui::SetScrollY(ImGui::GetScrollY() + scroll_offset);
				}

				bool draw_separator = true;
				for (int line_i = clipper.DisplayStart; line_i < clipper.DisplayEnd; line_i++) // display only visible items
				{
					int addr = line_i * bytes_per_line;
					ImGui::Text("%0*I64X: ", addr_digits_count, tabData->base_addr + addr);
					ImGui::SameLine();

					// Draw Hexadecimal
					float line_start_x = ImGui::GetCursorPosX();
					for (int n = 0; n < number_of_rows && addr < tabData->mem_size; n++, addr += bytes_per_row)
					{
						ImGui::SameLine(line_start_x + cell_width * n);

						if (DataEditingAddr == addr)
						{
							// Display text input on current byte
							ImGui::PushID(addr);
							needPopID = true;
							struct FuncHolder
							{
								// FIXME: We should have a way to retrieve the text edit cursor position more easily in the API, this is rather tedious.
								static int Callback(ImGuiTextEditCallbackData* data)
								{
									int* p_cursor_pos = (int*)data->UserData;
									if (!data->HasSelection())
										*p_cursor_pos = data->CursorPos;
									return 0;
								}
							};
							int cursor_pos = -1;
							bool data_write = false;
							if (DataEditingTakeFocus)
							{
								ImGui::SetKeyboardFocusHere();
								tabData->edit_addr = tabData->base_addr + addr;
								sprintf(AddrInput, "%0*I64X", addr_digits_count, tabData->base_addr + addr);
								switch (tabData->mode) {
								case(0):
									sprintf(DataInput, "%02X", tabData->mem_data[addr]);
									break;
								case(1):
									sprintf(DataInput, "%04X", *(uint16_t*)&tabData->mem_data[addr]);
									break;
								case(2):
									sprintf(DataInput, "%08X", *(uint32_t*)&tabData->mem_data[addr]);
									break;
								case(3):
									sprintf(DataInput, "%016I64X", *(uint64_t*)&tabData->mem_data[addr]);
									break;
								}
							}
							ImGui::PushItemWidth(input_width);
							ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_AlwaysInsertMode | ImGuiInputTextFlags_CallbackAlways;
							if (ImGui::InputText("##data", DataInput, sizeof(DataInput), flags, FuncHolder::Callback, &cursor_pos))
								data_write = data_next = true;
							else if (!DataEditingTakeFocus && !ImGui::IsItemActive())
								DataEditingAddr = -1;
							DataEditingTakeFocus = false;
							ImGui::PopItemWidth();
							if (cursor_pos >= 2 * bytes_per_row)
								data_write = data_next = true;
							if (data_write)
							{
								if (tabData->mode == 3) {
									uint64_t data;
									if (sscanf(DataInput, "%I64X", &data) == 1)
										*(uint64_t*)&tabData->mem_data[addr] = data;
								}
								else {
									int data;
									if (sscanf(DataInput, "%X", &data) == 1) {
										switch (tabData->mode) {
										case(0):
											tabData->mem_data[addr] = (unsigned char)data;
											break;
										case(1):
											*(uint16_t*)&tabData->mem_data[addr] = (uint16_t)data;
											break;
										case(2):
											*(uint32_t*)&tabData->mem_data[addr] = (uint32_t)data;
											break;
										};
									}
								}
							}
							ImGui::PopID();
							needPopID = false;
						}
						else
						{
							switch (tabData->mode) {
							case(0):
								ImGui::Text("%02X ", tabData->mem_data[addr]);
								break;
							case(1):
								ImGui::Text("%04X ", *(uint16_t*)&tabData->mem_data[addr]);
								break;
							case(2):
								ImGui::Text("%08X ", *(uint32_t*)&tabData->mem_data[addr]);
								break;
							case(3):
								ImGui::Text("%016I64X ", *(uint64_t*)&tabData->mem_data[addr]);
								break;
							};
							if (AllowEdits && ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
							{
								DataEditingTakeFocus = true;
								DataEditingAddr = addr;
							}
						}
					}

					ImGui::SameLine(line_start_x + cell_width * number_of_rows + glyph_width * 2);

					if (draw_separator)
					{
						ImVec2 screen_pos = ImGui::GetCursorScreenPos();
						ImGui::GetWindowDrawList()->AddLine(ImVec2(screen_pos.x - glyph_width, screen_pos.y - 9999), ImVec2(screen_pos.x - glyph_width, screen_pos.y + 9999), ImColor(ImGui::GetStyle().Colors[ImGuiCol_Border]));
						draw_separator = false;
					}

					// Draw ASCII values
					addr = line_i * bytes_per_line;
					for (int n = 0; n < bytes_per_line && addr < tabData->mem_size; n++, addr++)
					{
						if (n > 0) ImGui::SameLine();
						int c = tabData->mem_data[addr];
						char cstr[2] = { '.', '\0' };
						if (c >= 32 && c < 128) _snprintf(cstr, sizeof(cstr), "%c", c);
						if (DataEditingAddr != -1 && addr >= DataEditingAddr && addr < DataEditingAddr + bytes_per_row)
							ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), cstr);
						else ImGui::Text(cstr);
					}
				}
				clipper.End();
			}
			__except (BGDM_EXCEPTION("[MemoryEditor::Draw] access violation #2")) {

				// Set our size to 0 to skip memory access
				tabData->mem_size = 0;
				if (needPopID) {
					ImGui::PopID();
					needPopID = false;
				}
			}
			ImGui::PopStyleVar(2);
            ImGui::EndChild();

end:
            if (data_next && DataEditingAddr < tabData->mem_size)
            {
                DataEditingAddr = DataEditingAddr + bytes_per_row;
                DataEditingTakeFocus = true;
            }

            ImGui::Separator();

            ImGui::AlignFirstTextHeightToWidgets();
            ImGui::PushItemWidth(50);
            ImGui::PushAllowKeyboardFocus(false);
            int rows_backup = number_of_rows;
            if (ImGui::DragInt("##rows", &number_of_rows, 0.2f, 2, 32, "%.0f rows"))
            {
                ImVec2 new_window_size = ImGui::GetWindowSize();
                new_window_size.x += (number_of_rows - rows_backup) * (cell_width + glyph_width);
                ImGui::SetWindowSize(new_window_size);
				tabData->bytes_per_line = number_of_rows * bytes_per_row;
            }
            ImGui::PopAllowKeyboardFocus();
            ImGui::PopItemWidth();
            ImGui::SameLine();
            ImGui::Text("Range %0*I64X..%0*I64X",
				addr_digits_count, tabData->base_addr,
				addr_digits_count, tabData->base_addr + tabData->mem_size-1);
        }
        ImGui::End();
    }
};
