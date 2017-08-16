// Copied from here: https://github.com/ocornut/imgui/blob/a43a9e602a8f6059d6ef7977a7e0f4b8a84806e4/imgui.cpp
// Also here (older ver): https://gist.github.com/ocornut/9a55357df27d73cb8b34
#pragma once
#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

typedef int ImGuiColorEditFlags;    // color edit mode for ColorEdit*()     // enum ImGuiColorEditFlags_
									// Enumeration for ColorEdit3() / ColorEdit4() / ColorPicker3() / ColorPicker4()
enum ImGuiColorEditFlags_
{
	ImGuiColorEditFlags_Alpha = 1 << 0,   // ColorEdit/ColorPicker: show/edit Alpha component. Must be 0x01 for compatibility with old API taking bool
	ImGuiColorEditFlags_RGB = 1 << 1,   // ColorEdit: Choose one among RGB/HSV/HEX. User can still use the options menu to change. ColorPicker: Choose any combination or RGB/HSX/HEX.
	ImGuiColorEditFlags_HSV = 1 << 2,
	ImGuiColorEditFlags_HEX = 1 << 3,
	ImGuiColorEditFlags_NoPicker = 1 << 4,   // ColorEdit: Disable picker when clicking on colored square
	ImGuiColorEditFlags_NoOptions = 1 << 5,   // ColorEdit: Disable toggling options menu when right-clicking colored square
	ImGuiColorEditFlags_NoColorSquare = 1 << 6,   // ColorEdit: Disable colored square
	ImGuiColorEditFlags_NoSliders = 1 << 7,   // ColorEdit: Disable sliders, show only a button. ColorPicker: Disable all RGB/HSV/HEX sliders.
	ImGuiColorEditFlags_ModeMask_ = ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_HSV | ImGuiColorEditFlags_HEX
};

namespace ImGui {

IMGUI_API bool          ColorPicker3(const char* label, float col[3], ImGuiColorEditFlags flags = 0);
IMGUI_API bool          ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags = 0x01);

static __inline const char*      FindTextDisplayEnd(const char* text, const char* text_end = NULL);

// Find the optional ## from which we stop displaying text.
static __inline const char*  FindTextDisplayEnd(const char* text, const char* text_end)
{
	const char* text_display_end = text;
	if (!text_end)
		text_end = (const char*)-1;

	while (text_display_end < text_end && *text_display_end != '\0' && (text_display_end[0] != '#' || text_display_end[1] != '#'))
		text_display_end++;
	return text_display_end;
}

//-----------------------------------------------------------------------------
// HELPERS
//-----------------------------------------------------------------------------

#define IM_F32_TO_INT8_UNBOUND(_VAL)    ((int)((_VAL) * 255.0f + ((_VAL)>=0 ? 0.5f : -0.5f)))   // Unsaturated, for display purpose 
#define IM_F32_TO_INT8_SAT(_VAL)        ((int)(ImSaturate(_VAL) * 255.0f + 0.5f))               // Saturated, always output 0..255

bool ColorEdit4A(const char* label, float col[4], bool alpha)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const float w_full = CalcItemWidth();
	const float square_sz = (g.FontSize + style.FramePadding.y * 2.0f);

	ImGuiColorEditMode edit_mode = window->DC.ColorEditMode;
	if (edit_mode == ImGuiColorEditMode_UserSelect || edit_mode == ImGuiColorEditMode_UserSelectShowButton)
		edit_mode = g.ColorEditModeStorage.GetInt(id, 0) % 3;

	float f[4] = { col[0], col[1], col[2], col[3] };
	if (edit_mode == ImGuiColorEditMode_HSV)
		ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);

	int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

	int components = alpha ? 4 : 3;
	bool value_changed = false;

	BeginGroup();
	PushID(label);

	const bool hsv = (edit_mode == 1);
	switch (edit_mode)
	{
	case ImGuiColorEditMode_RGB:
	case ImGuiColorEditMode_HSV:
	{
		// RGB/HSV 0..255 Sliders
		const float w_items_all = w_full - (square_sz + style.ItemInnerSpacing.x);
		const float w_item_one = ImMax(1.0f, (float)(int)((w_items_all - (style.ItemInnerSpacing.x) * (components - 1)) / (float)components));
		const float w_item_last = ImMax(1.0f, (float)(int)(w_items_all - (w_item_one + style.ItemInnerSpacing.x) * (components - 1)));

		const bool hide_prefix = (w_item_one <= CalcTextSize("M:999").x);
		const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
		const char* fmt_table[3][4] =
		{
			{ "%3.0f",   "%3.0f",   "%3.0f",   "%3.0f" },
			{ "R:%3.0f", "G:%3.0f", "B:%3.0f", "A:%3.0f" },
			{ "H:%3.0f", "S:%3.0f", "V:%3.0f", "A:%3.0f" }
		};
		const char** fmt = hide_prefix ? fmt_table[0] : hsv ? fmt_table[2] : fmt_table[1];

		PushItemWidth(w_item_one);
		for (int n = 0; n < components; n++)
		{
			if (n > 0)
				SameLine(0, style.ItemInnerSpacing.x);
			if (n + 1 == components)
				PushItemWidth(w_item_last);
			value_changed |= DragInt(ids[n], &i[n], 1.0f, 0, 255, fmt[n]);
		}
		PopItemWidth();
		PopItemWidth();
	}
	break;
	case ImGuiColorEditMode_HEX:
	{
		// RGB Hexadecimal Input
		const float w_slider_all = w_full - square_sz;
		char buf[64];
		if (alpha)
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", i[0], i[1], i[2], i[3]);
		else
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", i[0], i[1], i[2]);
		PushItemWidth(w_slider_all - style.ItemInnerSpacing.x);
		if (InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
		{
			value_changed |= true;
			char* p = buf;
			while (*p == '#' || ImCharIsSpace(*p))
				p++;
			i[0] = i[1] = i[2] = i[3] = 0;
			if (alpha)
				sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]); // Treat at unsigned (%X is unsigned)
			else
				sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
		}
		PopItemWidth();
	}
	break;
	}

	SameLine(0, style.ItemInnerSpacing.x);

	const ImVec4 col_display(col[0], col[1], col[2], col[3]);
	if (ColorButton(col_display))
		g.ColorEditModeStorage.SetInt(id, (edit_mode + 1) % 3); // Don't set local copy of 'edit_mode' right away!

																// Recreate our own tooltip over's ColorButton() one because we want to display correct alpha here
	if (IsItemHovered())
		SetTooltip("Color:\n(%.2f,%.2f,%.2f,%.2f)\n#%02X%02X%02X%02X", col[0], col[1], col[2], col[3], IM_F32_TO_INT8_SAT(col[0]), IM_F32_TO_INT8_SAT(col[1]), IM_F32_TO_INT8_SAT(col[2]), IM_F32_TO_INT8_SAT(col[3]));

	if (window->DC.ColorEditMode == ImGuiColorEditMode_UserSelectShowButton)
	{
		SameLine(0, style.ItemInnerSpacing.x);
		const char* button_titles[3] = { "RGB", "HSV", "HEX" };
		if (ButtonEx(button_titles[edit_mode], ImVec2(0, 0), ImGuiButtonFlags_DontClosePopups))
			g.ColorEditModeStorage.SetInt(id, (edit_mode + 1) % 3); // Don't set local copy of 'edit_mode' right away!
	}

	const char* label_display_end = FindRenderedTextEnd(label);
	if (label != label_display_end)
	{
		SameLine(0, (window->DC.ColorEditMode == ImGuiColorEditMode_UserSelectShowButton) ? -1.0f : style.ItemInnerSpacing.x);
		TextUnformatted(label, label_display_end);
	}

	// Convert back
	for (int n = 0; n < 4; n++)
		f[n] = i[n] / 255.0f;
	if (edit_mode == 1)
		ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);

	if (value_changed)
	{
		col[0] = f[0];
		col[1] = f[1];
		col[2] = f[2];
		if (alpha)
			col[3] = f[3];
	}

	PopID();
	EndGroup();

	return value_changed;
}

bool ColorPicker3(const char* label, float col[3], ImGuiColorEditFlags flags)
{
	float col4[4] = { col[0], col[1], col[2], 1.0f };
	if (!ColorPicker4(label, col4, flags & ~ImGuiColorEditFlags_Alpha))
		return false;
	col[0] = col4[1]; col[1] = col4[1]; col[2] = col4[2];
	return true;
}

// ColorPicker v2.50 WIP 
// see https://github.com/ocornut/imgui/issues/346
// TODO: Missing color square
// TODO: English strings in context menu (see FIXME-LOCALIZATION)
bool ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Setup
	bool alpha = (flags & ImGuiColorEditFlags_Alpha) != 0;
	ImVec2 picker_pos = ImGui::GetCursorScreenPos();
	float bars_width = ImGui::GetWindowFontSize() * 1.0f;                                                           // Arbitrary smallish width of Hue/Alpha picking bars
	float sv_picker_size = ImMax(bars_width * 2, ImGui::CalcItemWidth() - (alpha ? 2 : 1) * (bars_width + style.ItemInnerSpacing.x)); // Saturation/Value picking box
	float bar0_pos_x = picker_pos.x + sv_picker_size + style.ItemInnerSpacing.x;
	float bar1_pos_x = bar0_pos_x + bars_width + style.ItemInnerSpacing.x;

	float H, S, V;
	ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], H, S, V);

	// Color matrix logic
	bool value_changed = false, hsv_changed = false;
	ImGui::BeginGroup();
	ImGui::InvisibleButton("sv", ImVec2(sv_picker_size, sv_picker_size));
	if (ImGui::IsItemActive())
	{
		S = ImSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size - 1));
		V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
		value_changed = hsv_changed = true;
	}

	// Hue bar logic
	ImGui::SetCursorScreenPos(ImVec2(bar0_pos_x, picker_pos.y));
	ImGui::InvisibleButton("hue", ImVec2(bars_width, sv_picker_size));
	if (ImGui::IsItemActive())
	{
		H = ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
		value_changed = hsv_changed = true;
	}

	// Alpha bar logic
	if (alpha)
	{
		ImGui::SetCursorScreenPos(ImVec2(bar1_pos_x, picker_pos.y));
		ImGui::InvisibleButton("alpha", ImVec2(bars_width, sv_picker_size));
		if (ImGui::IsItemActive())
		{
			col[3] = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
			value_changed = true;
		}
	}

	const char* label_display_end = FindTextDisplayEnd(label);
	if (label != label_display_end)
	{
		ImGui::SameLine(0, style.ItemInnerSpacing.x);
		ImGui::TextUnformatted(label, label_display_end);
	}

	// Convert back color to RGB
	if (hsv_changed)
		//ImGui::ColorConvertHSVtoRGB(H >= 1.0f ? H - 10 * 1e-6f : H, S > 0.0f ? S : 10 * 1e-6f, V > 0.0f ? V : 1e-6f, col[0], col[1], col[2]);
		ImGui::ColorConvertHSVtoRGB(H >= 1.0f ? H - 10 * 1e-6f : H, S > 0.0f ? S : 10 * 1e-2f, V > 0.0f ? V : 1e-2f, col[0], col[1], col[2]);

	// R,G,B and H,S,V slider color editor
	if (!(flags & ImGuiColorEditFlags_NoSliders))
	{
		if ((flags & ImGuiColorEditFlags_ModeMask_) == 0)
			flags = ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_HSV | ImGuiColorEditFlags_HEX;
		ImGui::PushItemWidth((alpha ? bar1_pos_x : bar0_pos_x) + bars_width - picker_pos.x);
		ImGuiColorEditFlags sub_flags = (alpha ? ImGuiColorEditFlags_Alpha : 0) | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoColorSquare;
		ImGuiColorEditMode edit_mode = ImGuiColorEditMode_RGB;
		if (flags & ImGuiColorEditFlags_HSV) edit_mode = ImGuiColorEditMode_HSV;
		if (flags & ImGuiColorEditFlags_HEX) edit_mode = ImGuiColorEditMode_HEX;
		ImGui::ColorEditMode(edit_mode);
		value_changed |= ImGui::ColorEdit4A("##hex", col, alpha);
		ImGui::PopItemWidth();
	}

	// Try to cancel hue wrap (after ColorEdit), if any
	if (value_changed)
	{
		float new_H, new_S, new_V;
		ImGui::ColorConvertRGBtoHSV(col[0], col[1], col[2], new_H, new_S, new_V);
		if (new_H <= 0 && H > 0)
		{
			if (new_V <= 0 && V != new_V)
				ImGui::ColorConvertHSVtoRGB(H, S, new_V <= 0 ? V * 0.5f : new_V, col[0], col[1], col[2]);
			else if (new_S <= 0)
				ImGui::ColorConvertHSVtoRGB(H, new_S <= 0 ? S * 0.5f : new_S, new_V, col[0], col[1], col[2]);
		}
	}

	// Render hue bar
	ImVec4 hue_color_f(1, 1, 1, 1);
	ImGui::ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
	ImU32 hue_colors[] = { IM_COL32(255,0,0,255), IM_COL32(255,255,0,255), IM_COL32(0,255,0,255), IM_COL32(0,255,255,255), IM_COL32(0,0,255,255), IM_COL32(255,0,255,255), IM_COL32(255,0,0,255) };
	for (int i = 0; i < 6; ++i)
	{
		draw_list->AddRectFilledMultiColor(
			ImVec2(bar0_pos_x, picker_pos.y + i * (sv_picker_size / 6)),
			ImVec2(bar0_pos_x + bars_width, picker_pos.y + (i + 1) * (sv_picker_size / 6)),
			hue_colors[i], hue_colors[i], hue_colors[i + 1], hue_colors[i + 1]);
	}
	float bar0_line_y = (float)(int)(picker_pos.y + H * sv_picker_size + 0.5f);
	draw_list->AddLine(ImVec2(bar0_pos_x - 1, bar0_line_y), ImVec2(bar0_pos_x + bars_width + 1, bar0_line_y), IM_COL32_WHITE);

	// Render alpha bar
	if (alpha)
	{
		float alpha = ImSaturate(col[3]);
		float bar1_line_y = (float)(int)(picker_pos.y + (1.0f - alpha) * sv_picker_size + 0.5f);
		draw_list->AddRectFilledMultiColor(ImVec2(bar1_pos_x, picker_pos.y), ImVec2(bar1_pos_x + bars_width, picker_pos.y + sv_picker_size), IM_COL32_WHITE, IM_COL32_WHITE, IM_COL32_BLACK, IM_COL32_BLACK);
		draw_list->AddLine(ImVec2(bar1_pos_x - 1, bar1_line_y), ImVec2(bar1_pos_x + bars_width + 1, bar1_line_y), IM_COL32_WHITE);
	}

	// Render color matrix
	ImU32 hue_color32 = ImGui::ColorConvertFloat4ToU32(hue_color_f);
	draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), IM_COL32_WHITE, hue_color32, hue_color32, IM_COL32_WHITE);
	draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), IM_COL32_BLACK_TRANS, IM_COL32_BLACK_TRANS, IM_COL32_BLACK, IM_COL32_BLACK);

	// Render cross-hair
	const float CROSSHAIR_SIZE = 7.0f;
	ImVec2 p((float)(int)(picker_pos.x + S * sv_picker_size + 0.5f), (float)(int)(picker_pos.y + (1 - V) * sv_picker_size + 0.5f));
	draw_list->AddLine(ImVec2(p.x - CROSSHAIR_SIZE, p.y), ImVec2(p.x - 2, p.y), IM_COL32_WHITE);
	draw_list->AddLine(ImVec2(p.x + CROSSHAIR_SIZE, p.y), ImVec2(p.x + 2, p.y), IM_COL32_WHITE);
	draw_list->AddLine(ImVec2(p.x, p.y + CROSSHAIR_SIZE), ImVec2(p.x, p.y + 2), IM_COL32_WHITE);
	draw_list->AddLine(ImVec2(p.x, p.y - CROSSHAIR_SIZE), ImVec2(p.x, p.y - 2), IM_COL32_WHITE);
	ImGui::EndGroup();

	return value_changed;
}


bool ColorPicker(const char* label, float col[4])
{
	ImGuiContext& g = *GImGui;
	ImGuiStyle& style = ImGui::GetStyle();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Setup
	float bars_width = ImGui::GetWindowFontSize() * 1.0f;                                                           // Arbitrary smallish width of Hue/Alpha picking bars
	float picker_size = ImMax(bars_width * 2, ImGui::CalcItemWidth() - 1 * (bars_width + style.ItemInnerSpacing.x)); // Saturation/Value picking box
	const float windowWidth = picker_size;
	const float smallWidth = 20.0f;

	ImU32 black = ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1));
	ImU32 white = ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1));
	static float hue, sat, val;
	bool bRet = false;

	ColorConvertRGBtoHSV(col[0], col[1], col[2], hue, sat, val);

	ImGuiWindow* colorWindow = GetCurrentWindow();

	//Actual color
	ImGui::BeginChild("ColorAlpha", ImVec2(windowWidth, smallWidth + 5), false);
	{
		ImGuiWindow* window = GetCurrentWindow();
		static float rounding = 3.0f;
		ImVec2 size(20.0f, 20.0f);
		ImVec2 pos(window->Pos);
		ImVec4 colorRGB(*(ImVec4*)col);
		window->DrawList->AddRectFilled(pos, pos + size, ColorConvertFloat4ToU32(colorRGB), rounding);
		pos.x += size.x + 10.0f;
		colorRGB.w = 1;
		window->DrawList->AddRectFilled(pos, pos + size, ColorConvertFloat4ToU32(colorRGB), rounding);
		pos.x += size.x + 10.0f;
		ImVec4 colorAlpha;
		colorAlpha.x = colorAlpha.y = colorAlpha.z = col[3];
		colorAlpha.w = 1;
		window->DrawList->AddRectFilled(pos, pos + size, ColorConvertFloat4ToU32(colorAlpha), rounding);
		ImGui::EndChild();
	}

	//Saturation quad
	ImGui::Separator();
	{
		const float quadSize = windowWidth - smallWidth - style.ItemSpacing.x;
		// Hue Saturation Value
		ImGui::BeginChild("ValueSaturationQuad", ImVec2(quadSize, quadSize), false);
		{
			const int step = 5;
			ImVec2 pos = ImVec2(0, 0);
			ImGuiWindow* window = GetCurrentWindow();

			ImVec4 c00(1, 1, 1, 1);
			ImVec4 c10(1, 1, 1, 1);
			ImVec4 c01(1, 1, 1, 1);
			ImVec4 c11(1, 1, 1, 1);
			for (int y = 0; y < step; y++) {
				for (int x = 0; x < step; x++) {
					float s0 = (float)x / (float)step;
					float s1 = (float)(x + 1) / (float)step;
					float v0 = 1.0f - (float)(y) / (float)step;
					float v1 = 1.0f - (float)(y + 1) / (float)step;


					ColorConvertHSVtoRGB(hue, s0, v0, c00.x, c00.y, c00.z);
					ColorConvertHSVtoRGB(hue, s1, v0, c10.x, c10.y, c10.z);
					ColorConvertHSVtoRGB(hue, s0, v1, c01.x, c01.y, c01.z);
					ColorConvertHSVtoRGB(hue, s1, v1, c11.x, c11.y, c11.z);

					window->DrawList->AddRectFilledMultiColor(window->Pos + pos, window->Pos + pos + ImVec2(quadSize / step, quadSize / step),
						ColorConvertFloat4ToU32(c00),
						ColorConvertFloat4ToU32(c10),
						ColorConvertFloat4ToU32(c11),
						ColorConvertFloat4ToU32(c01));

					pos.x += quadSize / step;
				}
				pos.x = 0;
				pos.y += quadSize / step;
			}

			//window->DrawList->AddCircle(window->Pos + ImVec2(sat, 1-val)*quadSize, 4, val<0.5f?white:black, 4);

			const ImGuiID id = window->GetID("ValueSaturationQuad");
			ImRect bb(window->Pos, window->Pos + window->Size);
			bool hovered, held;
			bool pressed = ButtonBehavior(bb, id, &hovered, &held);
			if (held)
			{
				ImVec2 pos = g.IO.MousePos - window->Pos;
				sat = ImSaturate(pos.x / (float)quadSize);
				val = 1 - ImSaturate(pos.y / (float)quadSize);
				ColorConvertHSVtoRGB(hue, sat, val, col[0], col[1], col[2]);
				bRet = true;
			}

		}
		ImGui::EndChild();	// ValueSaturationQuad

		ImGui::SameLine();

		//Vertical tint
		ImGui::BeginChild("Tint", ImVec2(20, quadSize), false);
		{
			const float step = 8.0f;
			const float width = 20.0f;
			ImGuiWindow* window = GetCurrentWindow();
			ImVec2 pos(0, 0);
			ImVec4 c0(1, 1, 1, 1);
			ImVec4 c1(1, 1, 1, 1);
			for (int y = 0; y < step; y++) {
				float tint0 = (float)(y) / (float)step;
				float tint1 = (float)(y + 1) / (float)step;
				ColorConvertHSVtoRGB(tint0, 1.0, 1.0, c0.x, c0.y, c0.z);
				ColorConvertHSVtoRGB(tint1, 1.0, 1.0, c1.x, c1.y, c1.z);

				window->DrawList->AddRectFilledMultiColor(window->Pos + pos, window->Pos + pos + ImVec2(width, quadSize / step),
					ColorConvertFloat4ToU32(c0),
					ColorConvertFloat4ToU32(c0),
					ColorConvertFloat4ToU32(c1),
					ColorConvertFloat4ToU32(c1));

				pos.y += quadSize / step;
			}

			window->DrawList->AddCircle(window->Pos + ImVec2(10, hue*quadSize), 4, black, 4);
			//window->DrawList->AddLine(window->Pos + ImVec2(0, hue*quadSize), window->Pos + ImVec2(width, hue*quadSize), ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)));
			bool hovered, held;
			const ImGuiID id = window->GetID("Tint");
			ImRect bb(window->Pos, window->Pos + window->Size);
			bool pressed = ButtonBehavior(bb, id, &hovered, &held);
			if (held)
			{
				ImVec2 pos = g.IO.MousePos - window->Pos;
				hue = ImClamp(pos.y / (float)quadSize, 0.0f, 1.0f);
				ColorConvertHSVtoRGB(hue >= 1.0f ? hue - 10 * 1e-6f : hue, sat > 0.0f ? sat : 10 * 1e-2f, val > 0.0f ? val : 1e-2f, col[0], col[1], col[2]);
				bRet = true;
			}
		}
		ImGui::EndChild(); // "Tint"

		//Sliders
		ImGui::Separator();
		{
			float r = ImSaturate(col[0])*255.f;
			float g = ImSaturate(col[1])*255.f;
			float b = ImSaturate(col[2])*255.f;
			float a = ImSaturate(col[3])*255.f;

			ImGui::PushItemWidth(quadSize);
			bRet |= ImGui::SliderFloat("R", &r, 0.0f, 255.0f, "%.0f");
			bRet |= ImGui::SliderFloat("G", &g, 0.0f, 255.0f, "%.0f");
			bRet |= ImGui::SliderFloat("B", &b, 0.0f, 255.0f, "%.0f");
			bRet |= ImGui::SliderFloat("A", &a, 0.0f, 255.0f, "%.0f");
			ImGui::PopItemWidth();

			col[0] = r / 255.f;
			col[1] = g / 255.f;
			col[2] = b / 255.f;
			col[3] = a / 255.f;

			//ColorConvertRGBtoHSV(s_color.x, s_color.y, s_color.z, tint, sat, val);*/
		}
	}

	return bRet;
}

}	// namespace ImGui