﻿#include <CustomWheel.h>
#include <Wheel.h>
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include <filesystem>
#include <fstream>
#include <imgui/examples/imgui_impl_dx9.h>

#include <ImGuiPopup.h>

#include <TGA.h>

namespace GW2Radial
{

float CalcText(ImFont* font, const std::wstring& text)
{
	cref txt = utf8_encode(text);
	
	auto sz = font->CalcTextSizeA(100.f, FLT_MAX, 0.f, txt.c_str());

	return sz.x;
}

IDirect3DTexture9* DrawText(IDirect3DDevice9* dev, ImFont* font, float fontSize, const std::wstring& text)
{
	const uint fgColor = 0xFFFFFFFF;
	const uint bgColor = 0x00000000;

	cref txt = utf8_encode(text);
	
	auto sz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, txt.c_str());

	ImVec2 clip(1024.f, fontSize);

	float xOff = (clip.x - sz.x) * 0.5f;

	ImDrawList imDraw(ImGui::GetDrawListSharedData());
	imDraw.Clear();
	imDraw.PushClipRect(ImVec2(0.f, 0.f), clip);
	imDraw.PushTextureID(font->ContainerAtlas->TexID);
	imDraw.AddText(font, fontSize, ImVec2(xOff, 0.f), fgColor, txt.c_str());
	
	auto& io = ImGui::GetIO();
	auto oldDisplaySize = io.DisplaySize;
	io.DisplaySize = clip;

	IDirect3DTexture9* tex = nullptr;
	dev->CreateTexture(uint(clip.x), uint(clip.y), 1,
 D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, nullptr);

	auto hr = dev->BeginScene();

	IDirect3DSurface9* surf;
	tex->GetSurfaceLevel(0, &surf);
	dev->SetRenderTarget(0, surf);

	dev->Clear(1, nullptr, D3DCLEAR_TARGET, bgColor, 0.f, 0);

	ImDrawList* imDraws[] = { &imDraw };
	ImDrawData imData;
	imData.Valid = true;
	imData.CmdLists = imDraws;
	imData.CmdListsCount = 1;
	imData.TotalIdxCount = imDraw.IdxBuffer.size();
	imData.TotalVtxCount = imDraw.VtxBuffer.size();
	imData.DisplayPos = imDraw.GetClipRectMin();
	imData.DisplaySize = imDraw.GetClipRectMax();
	ImGui_ImplDX9_RenderDrawData(&imData);
	
	dev->SetRenderTarget(0, nullptr);

#if 0
	{
	    IDirect3DSurface9* surf2;
	    GW2_ASSERT(SUCCEEDED(dev->CreateOffscreenPlainSurface(uint(clip.x), uint(clip.y), D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf2, nullptr)));

	    GW2_ASSERT(SUCCEEDED(dev->GetRenderTargetData(surf, surf2)));

	    D3DLOCKED_RECT rect;
	    GW2_ASSERT(SUCCEEDED(surf2->LockRect(&rect, nullptr, D3DLOCK_READONLY)));
	    std::span<byte> rectSpan((byte*)rect.pBits, uint(clip.x) * uint(clip.y) * 4);
	    cref tgaData = SaveTGA(rectSpan, uint(clip.x), uint(clip.y), 32, rect.Pitch);
	    std::ofstream of((text + L".tga").c_str(), std::ofstream::binary | std::ofstream::trunc);
	    of.write((char*)tgaData.data(), tgaData.size());

	    surf2->UnlockRect();
		surf2->Release();
	}
#endif

	io.DisplaySize = oldDisplaySize;

	surf->Release();

	if(hr != D3DERR_INVALIDCALL)
	    dev->EndScene();

	return tex;
}

IDirect3DTexture9* LoadCustomTexture(IDirect3DDevice9* dev, const std::filesystem::path& path)
{
	IDirect3DTexture9* tex = nullptr;
	if(path.extension() == L".dds")
        GW2_ASSERT(SUCCEEDED(DirectX::CreateDDSTextureFromFile(dev, path.wstring().c_str(), &tex)));
	else
        GW2_ASSERT(SUCCEEDED(DirectX::CreateWICTextureFromFile(dev, path.wstring().c_str(), &tex)));
	return tex;
}

CustomWheelsManager::CustomWheelsManager(std::vector<std::unique_ptr<Wheel>>& wheels, ImFont* font, IDirect3DDevice9* dev)
    : wheels_(wheels), device_(dev), font_(font)
{
	Reload();
}

void CustomWheelsManager::Draw()
{
	if(failedLoads_.empty())
		return;

	cref err = failedLoads_.back();

	ImGuiPopup("Custom wheel configuration could not be loaded!")
        .Position({0.5f, 0.45f}).Size({0.35f, 0.2f})
        .Display([err](const ImVec2&)
			{
				ImGui::Text("Could not load custom wheel. Error message:");
				ImGui::TextWrapped(utf8_encode(err).c_str());
			}, [&]() { failedLoads_.pop_back(); });
}

std::unique_ptr<Wheel> CustomWheelsManager::BuildWheel(const std::filesystem::path& configPath)
{
	cref dataFolder = configPath.parent_path();

	auto fail = [&](const wchar_t* error)
	{
		failedLoads_.push_back(error + (L": '" + configPath.wstring() + L"'"));
		return nullptr;
	};

	CSimpleIniA ini(true);
	cref loadResult = ini.LoadFile(configPath.wstring().c_str());
	if(loadResult != SI_OK)
		return fail(L"Invalid INI file");

	cref general = *ini.GetSection("General");
	cref wheelDisplayName = general.find("display_name");
	cref wheelNickname = general.find("nickname");
	
	if(wheelDisplayName == general.end())
		return fail(L"Missing field display_name");
	if(wheelNickname == general.end())
		return fail(L"Missing field nickname");

	if(std::any_of(customWheels_.begin(), customWheels_.end(), [&](cref w)
	{
	    return w->nickname() == wheelNickname->second;
	}))
		return fail((L"Nickname " + utf8_decode(std::string(wheelNickname->second)) + L" already exists").c_str());

	auto wheel = std::make_unique<Wheel>(IDR_BG, IDR_WIPEMASK, wheelNickname->second, wheelDisplayName->second, device_);

	uint baseId = customWheelNextId_;

	std::list<CSimpleIniA::Entry> sections;
	ini.GetAllSections(sections);
	sections.sort(CSimpleIniA::Entry::LoadOrder());
	std::vector<CustomElementSettings> elements; elements.reserve(sections.size());
	float maxTextWidth = 0.f;
	for(cref sec : sections)
	{
	    if(_stricmp(sec.pItem, "General") == 0)
			continue;

		cref element = *ini.GetSection(sec.pItem);
		
		cref elementName = element.find("name");
		cref elementColor = element.find("color");
		cref elementIcon = element.find("icon");
		cref elementShadow = element.find("shadow_strength");
		cref elementColorize = element.find("colorize_strength");

		fVector4 color { 1.f, 1.f, 1.f, 1.f };
		if(elementColor != element.end())
		{
			std::array<std::string, 3> colorElems;
			SplitString(elementColor->second, ",", colorElems.begin());

			size_t i = 0;
			for(cref c : colorElems)
				reinterpret_cast<float*>(&color)[i++] = float(atof(c.c_str()) / 255.f);
		}

	    CustomElementSettings ces;
		ces.category = wheel->nickname();
		ces.nickname = ToLower(wheel->nickname()) + "_" + ToLower(std::string(sec.pItem));
		ces.name = elementName == element.end() ? sec.pItem : elementName->second;
		ces.color = color;
		ces.id = baseId++;
		ces.shadow = elementShadow == element.end() ? 1.f : atof(elementShadow->second);
		ces.colorize = elementColorize == element.end() ? 1.f : atof(elementColorize->second);

		if(elementIcon != element.end())
			ces.texture = LoadCustomTexture(device_, dataFolder / utf8_decode(elementIcon->second));
		else
			maxTextWidth = std::max(maxTextWidth, CalcText(font_, utf8_decode(ces.name)));

		elements.push_back(ces);

		GW2_ASSERT(baseId < customWheelNextId_ + CustomWheelIdStep);
	}

	float desiredFontSize = 1024.f / maxTextWidth * 100.f;

	for(auto& ces : elements)
	{
	    if(ces.texture == nullptr)
		    ces.texture = DrawText(device_, font_, desiredFontSize, utf8_decode(ces.name));

	    wheel->AddElement(std::make_unique<CustomElement>(ces, device_));
	}

	customWheelNextId_ += CustomWheelIdStep;

	return std::move(wheel);
}

void CustomWheelsManager::Reload()
{
	failedLoads_.clear();
	customWheelNextId_ = CustomWheelStartId;

    if(!customWheels_.empty()) {
	    wheels_.erase(std::remove_if(wheels_.begin(), wheels_.end(), [&](cref ptr)
	    {
		    return std::find(customWheels_.begin(), customWheels_.end(), ptr.get()) != customWheels_.end();
	    }), wheels_.end());

		customWheels_.clear();
	}

	ImGui_ImplDX9_NewFrame();

	cref folderBase = ConfigurationFile::i()->folder() + L"custom\\";

	for(cref entry : std::filesystem::directory_iterator(folderBase))
	{
	    if(!entry.is_directory())
			continue;

		std::filesystem::path configFile = entry.path() / L"config.ini";
		if(!std::filesystem::exists(configFile))
			continue;

		auto wheel = BuildWheel(configFile.wstring());
		if(wheel)
		{
		    wheels_.push_back(std::move(wheel));
		    customWheels_.push_back(wheels_.back().get());
		}
	}
}

CustomElement::CustomElement(const CustomElementSettings& ces, IDirect3DDevice9* dev)
	: WheelElement(ces.id, ces.nickname, ces.category, ces.name, dev, ces.texture), color_(ces.color)
{
    shadowStrength_ = ces.shadow;
	colorizeAmount_ = ces.colorize;
}

fVector4 CustomElement::color()
{
	return color_;
}

}
