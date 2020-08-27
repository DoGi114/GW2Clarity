#include <ActivationKeybind.h>
#include <ImGuiExtensions.h>
#include <Core.h>
#include "../imgui/imgui_internal.h"
#include <Input.h>

ImVec2 operator*(const ImVec2& a, const ImVec2& b)
{
	return ImVec2(a.x * b.x, a.y * b.y);
}
ImVec2 operator*(const ImVec2& a, float b)
{
	return ImVec2(a.x * b, a.y * b);
}
ImVec2 operator-(const ImVec2& a, const ImVec2& b)
{
	return ImVec2(a.x - b.x, a.y - b.y);
}

ImVec2 operator*=(ImVec2 & a, const ImVec2 & b)
{
	a = a * b;
	return a;
}

ImVec4 operator/(const ImVec4& v, const float f)
{
	return { v.x / f, v.y / f, v.z / f, v.w / f };
}

std::map<void*, int> specialIds;

void ImGuiKeybindInput(GW2Radial::Keybind& setting, const char* tooltip)
{
	std::string suffix = "##" + setting.nickname();

	float windowWidth = ImGui::GetWindowWidth() - ImGuiHelpTooltipSize();

	ImGui::PushItemWidth(windowWidth * 0.45f);

	int popcount = 1;
	if (setting.isBeingModified())
	{
		popcount = 3;
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(201, 215, 255, 200) / 255.f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0, 0, 0, 1));
	}
	else
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1));

	ImGui::InputText(suffix.c_str(), setting.keysDisplayStringArray().data(), setting.keysDisplayStringArray().size(), ImGuiInputTextFlags_ReadOnly);

	ImGui::PopItemWidth();

	ImGui::PopStyleColor(popcount);

	ImGui::SameLine();

	if (!setting.isBeingModified() && ImGui::Button(("Set" + suffix).c_str(), ImVec2(windowWidth * 0.1f, 0.f)))
	{
		setting.isBeingModified(true);
		setting.keysDisplayStringArray().at(0) = '\0';
	}
	else if (setting.isBeingModified() && ImGui::Button(("Clear" + suffix).c_str(), ImVec2(windowWidth * 0.1f, 0.f)))
	{
		setting.isBeingModified(false);
		setting.scanCodes(std::set<GW2Radial::ScanCode>());
	}

	ImGui::SameLine();

	ImGui::PushItemWidth(windowWidth * 0.5f);

	auto* activationSetting = dynamic_cast<GW2Radial::ActivationKeybind*>(&setting);

	if(activationSetting && activationSetting->isConflicted())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
		ImGui::Text((setting.displayName() + "[!]").c_str());
		ImGui::PopStyleColor();
	}
	else
		ImGui::Text(setting.displayName().c_str());

	ImGui::PopItemWidth();

	if(tooltip)
	    ImGuiHelpTooltip(tooltip);
}

void ImGuiTitle(const char * text)
{
	ImGui::PushFont(GW2Radial::Core::i()->fontBlack());
	ImGui::TextUnformatted(text);
	ImGui::Separator();
	ImGui::PopFont();
}

float ImGuiHelpTooltipSize() {
    return ImGui::CalcTextSize("(?)").x + ImGui::GetStyle().ItemSpacing.x + 1.f;
}

void ImGuiHelpTooltip(const char* desc)
{
	ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGuiHelpTooltipSize() - ImGui::GetScrollX() - ImGui::GetStyle().ScrollbarSize);
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}