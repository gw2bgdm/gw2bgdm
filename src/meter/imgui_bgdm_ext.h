#pragma once
#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

namespace ImGui {


IMGUI_API int GetColumnsDataSize()
{
	ImGuiWindow* window = GetCurrentWindow();
	return window->DC.ColumnsData.Size;
}

IMGUI_API bool MenuItem(const char* label, const char* shortcut, bool selected, bool enabled, ImGuiSelectableFlags flags);
IMGUI_API bool MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled, ImGuiSelectableFlags flags);
IMGUI_API bool MenuItemNoPopupClose(const char* label, const char* shortcut = NULL, bool selected = false, bool enabled = true);
IMGUI_API bool MenuItemNoPopupClose(const char* label, const char* shortcut, bool* p_selected, bool enabled = true);
IMGUI_API bool SelectableNoPopupClose(const char* label, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0, 0));
IMGUI_API bool SelectableNoPopupClose(const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0, 0));

// Please note that you can tweak the "format" argument if you want to add a prefix (or a suffix) piece of text to the text that appears at the right of the bar.
// returns the value "fraction" in 0.f-1.f.
// It does not need any ID.
IMGUI_API float ProgressBar(const char* optionalPrefixText, float value, const float minValue = 0.f, const float maxValue = 1.f, const char* format = "%1.0f%%", const ImVec2& sizeOfBarWithoutTextInPixels = ImVec2(-1, -1),
	const ImVec4& colorLeft = ImVec4(0, 1, 0, 0.8), const ImVec4& colorRight = ImVec4(0, 0.4, 0, 0.8), const ImVec4& colorBorder = ImVec4(0.25, 0.25, 1.0, 1));

void TestProgressBar();

bool SelectableNoPopupClose(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size)
{
	flags |= ImGuiSelectableFlags_DontClosePopups;
	return Selectable(label, selected, flags, size);
}

bool SelectableNoPopupClose(const char* label, bool *p_selected, ImGuiSelectableFlags flags, const ImVec2& size)
{
	flags |= ImGuiSelectableFlags_DontClosePopups;
	return Selectable(label, p_selected, flags, size);
}

bool MenuItemNoPopupClose(const char* label, const char* shortcut, bool selected, bool enabled)
{
	return MenuItem(label, shortcut, selected, enabled, ImGuiSelectableFlags_DontClosePopups);
}

bool MenuItemNoPopupClose(const char* label, const char* shortcut, bool* p_selected, bool enabled)
{
	return MenuItem(label, shortcut, p_selected, enabled, ImGuiSelectableFlags_DontClosePopups);
}

bool MenuItem(const char* label, const char* shortcut, bool selected, bool enabled, ImGuiSelectableFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	ImVec2 pos = window->DC.CursorPos;
	ImVec2 label_size = CalcTextSize(label, NULL, true);
	ImVec2 shortcut_size = shortcut ? CalcTextSize(shortcut, NULL) : ImVec2(0.0f, 0.0f);
	float w = window->MenuColumns.DeclColumns(label_size.x, shortcut_size.x, (float)(int)(g.FontSize * 1.20f)); // Feedback for next frame
	float extra_w = ImMax(0.0f, GetContentRegionAvail().x - w);

	// HACK to prevent setting ImGuiButtonFlags_PressedOnRelease inside ImGui::Selectable()
	if (flags & ImGuiSelectableFlags_DontClosePopups) flags |= ImGuiSelectableFlags_Menu;
	else flags |= ImGuiSelectableFlags_MenuItem;

	bool pressed = Selectable(label, false, flags | /*ImGuiSelectableFlags_MenuItem |*/ ImGuiSelectableFlags_DrawFillAvailWidth | (enabled ? 0 : ImGuiSelectableFlags_Disabled), ImVec2(w, 0.0f));
	if (shortcut_size.x > 0.0f)
	{
		PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
		RenderText(pos + ImVec2(window->MenuColumns.Pos[1] + extra_w, 0.0f), shortcut, NULL, false);
		PopStyleColor();
	}

	if (selected)
		RenderCheckMark(pos + ImVec2(window->MenuColumns.Pos[2] + extra_w + g.FontSize * 0.20f, 0.0f), GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled));

	return pressed;
}

bool MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled, ImGuiSelectableFlags flags)
{
	if (MenuItem(label, shortcut, p_selected ? *p_selected : false, enabled, flags))
	{
		if (p_selected)
			*p_selected = !*p_selected;
		return true;
	}
	return false;
}

float ProgressBar(const char *optionalPrefixText, float value, const float minValue, const float maxValue, const char *format, const ImVec2 &sizeOfBarWithoutTextInPixels, const ImVec4 &colorLeft, const ImVec4 &colorRight, const ImVec4 &colorBorder) {
	if (value<minValue) value = minValue;
	else if (value>maxValue) value = maxValue;
	const float valueFraction = (maxValue == minValue) ? 1.0f : ((value - minValue) / (maxValue - minValue));
	const bool needsPercConversion = strstr(format, "%%") != NULL;

	ImVec2 size = sizeOfBarWithoutTextInPixels;
	if (size.x <= 0) size.x = ImGui::GetWindowWidth()*0.25f;
	if (size.y <= 0) size.y = ImGui::GetTextLineHeightWithSpacing(); // or without

	const ImFontAtlas* fontAtlas = ImGui::GetIO().Fonts;

	if (optionalPrefixText && strlen(optionalPrefixText)>0) {
		ImGui::AlignFirstTextHeightToWidgets();
		ImGui::Text("%s", optionalPrefixText);
		ImGui::SameLine();
	}

	if (valueFraction>0) {
		ImGui::Image(fontAtlas->TexID, ImVec2(size.x*valueFraction, size.y), fontAtlas->TexUvWhitePixel, fontAtlas->TexUvWhitePixel, colorLeft, colorBorder);
	}
	if (valueFraction<1) {
		if (valueFraction>0) ImGui::SameLine(0, 0);
		ImGui::Image(fontAtlas->TexID, ImVec2(size.x*(1.f - valueFraction), size.y), fontAtlas->TexUvWhitePixel, fontAtlas->TexUvWhitePixel, colorRight, colorBorder);
	}
	ImGui::SameLine();

	ImGui::Text(format, needsPercConversion ? (valueFraction*100.f + 0.0001f) : value);
	return valueFraction;
}

void TestProgressBar() {
	const float time = ((float)(((unsigned int)(ImGui::GetTime()*1000.f)) % 50000) - 25000.f) / 25000.f;
	float progress = (time>0 ? time : -time);
	// No IDs needed for ProgressBars:
	ImGui::ProgressBar("ProgressBar", progress);
	ImGui::ProgressBar("ProgressBar", 1.f - progress);
	ImGui::ProgressBar("", 500 + progress * 1000, 500, 1500, "%4.0f (absolute value in [500,1500] and fixed bar size)", ImVec2(150, -1));
	ImGui::ProgressBar("", 500 + progress * 1000, 500, 1500, "%3.0f%% (same as above, but with percentage and new colors)", ImVec2(150, -1), ImVec4(0.7, 0.7, 1, 1), ImVec4(0.05, 0.15, 0.5, 0.8), ImVec4(0.8, 0.8, 0, 1));
	// This one has just been added to ImGui:
	//char txt[48]="";sprintf(txt,"%3d%% (ImGui default progress bar)",(int)(progress*100));
	//ImGui::ProgressBar(progress,ImVec2(0,0),txt);
}

}