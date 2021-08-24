#pragma once
#include <Main.h>
#include <imgui.h>
#include <Keybind.h>
#include <ConfigurationOption.h>

ImVec2 operator*(const ImVec2& a, const ImVec2& b);
ImVec2 operator*(const ImVec2& a, float b);
ImVec2 operator-(const ImVec2& a, const ImVec2& b);
ImVec2 operator*=(ImVec2& a, const ImVec2& b);

ImVec4 operator/(const ImVec4& v, float f);
bool ImGuiKeybindInput(GW2Radial::Keybind& setting, bool beingModified, const char* tooltip);

template<typename F, typename T, typename... Args>
bool ImGuiConfigurationWrapper(F fct, const char* name, GW2Radial::ConfigurationOption<T>& value, Args&&... args)
{
	if(fct(name, &value.value(), std::forward<Args>(args)...)) {
		value.ForceSave();
		return true;
	}

	return false;
}

template<typename F, typename T, typename... Args>
bool ImGuiConfigurationWrapper(F fct, GW2Radial::ConfigurationOption<T>& value, Args&&... args)
{
	if(fct(value.displayName().c_str(), &value.value(), std::forward<Args>(args)...)) {
		value.ForceSave();
		return true;
	}

	return false;
}

void ImGuiTitle(const char * text);
float ImGuiHelpTooltipSize();
void ImGuiHelpTooltip(const char* desc);

void ImGuiDisable(float alpha = 0.6f);
void ImGuiDisableEnd();