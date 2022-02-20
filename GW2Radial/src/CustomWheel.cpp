﻿#include <CustomWheel.h>
#include <Wheel.h>
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include <filesystem>
#include <fstream>
#include <ImGuiPopup.h>
#include <FileSystem.h>
#include <backends/imgui_impl_dx11.h>

namespace GW2Radial
{

float CalcText(ImFont* font, const std::wstring& text)
{
	cref txt = utf8_encode(text);
	
	auto sz = font->CalcTextSizeA(100.f, FLT_MAX, 0.f, txt.c_str());

	return sz.x;
}

RenderTarget MakeTextTexture(ID3D11Device* dev, float fontSize)
{
	RenderTarget rt;
	D3D11_TEXTURE2D_DESC desc;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Width = 1024;
	desc.Height = uint(fontSize);
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	dev->CreateTexture2D(&desc, nullptr, &rt.texture);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	dev->CreateShaderResourceView(rt.texture.Get(), &srvDesc, &rt.srv);

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	dev->CreateRenderTargetView(rt.texture.Get(), &rtvDesc, &rt.rtv);

	return rt;
}

void DrawText(ID3D11DeviceContext* ctx, RenderTarget& rt, ImFont* font, float fontSize, const std::wstring& text)
{
	const uint fgColor = 0xFFFFFFFF;
	const uint bgColor = 0x00000000;

	cref txt = utf8_encode(text);
	
	auto sz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, txt.c_str());

	ImVec2 clip(1024.f, fontSize);

	float xOff = (clip.x - sz.x) * 0.5f;

	ImDrawList imDraw(ImGui::GetDrawListSharedData());
	imDraw.AddDrawCmd();
	imDraw.PushClipRect(ImVec2(0.f, 0.f), clip);
	imDraw.PushTextureID(font->ContainerAtlas->TexID);
	imDraw.AddText(font, fontSize, ImVec2(xOff, 0.f), fgColor, txt.c_str());
	
	auto& io = ImGui::GetIO();
	auto oldDisplaySize = io.DisplaySize;
	io.DisplaySize = clip;

	ID3D11RenderTargetView* oldRt;
	ID3D11DepthStencilView* oldDs;
	ctx->OMGetRenderTargets(1, &oldRt, &oldDs);
	ctx->OMSetRenderTargets(1, &rt.rtv, nullptr);

	float clearBlack[] = { 0.f, 0.f, 0.f, 0.f };
	ctx->ClearRenderTargetView(rt.rtv.Get(), clearBlack);

	ImDrawList* imDraws[] = { &imDraw };
	ImDrawData imData;
	imData.Valid = true;
	imData.CmdLists = imDraws;
	imData.CmdListsCount = 1;
	imData.TotalIdxCount = imDraw.IdxBuffer.Size;
	imData.TotalVtxCount = imDraw.VtxBuffer.Size;
	imData.DisplayPos = ImVec2(0.0f, 0.0f);
	imData.DisplaySize = io.DisplaySize;

	ImGui_ImplDX11_RenderDrawData(&imData);

	ctx->OMSetRenderTargets(1, &oldRt, oldDs);

	io.DisplaySize = oldDisplaySize;
}

Texture2D LoadCustomTexture(ID3D11Device* dev, const std::filesystem::path& path)
{
	cref data = FileSystem::ReadFile(path);
	if(data.empty())
		return {};

	try {
		Texture2D tex;
		ComPtr<ID3D11Resource> res;
		HRESULT hr = S_OK;
	    if(path.extension() == L".dds")
            hr = DirectX::CreateDDSTextureFromMemory(dev, data.data(), data.size(), &res, &tex.srv);
	    else
            hr = DirectX::CreateWICTextureFromMemory(dev, data.data(), data.size(), &res, &tex.srv);
		if(res)
			res->QueryInterface(tex.texture.GetAddressOf());

		if(!SUCCEEDED(hr))
		    FormattedMessageBox(L"Could not load custom radial menu image '%s': 0x%x.", L"Custom Menu Error", path.wstring().c_str(), hr);

	    return tex;
	} catch(...) {
		FormattedMessageBox(L"Could not load custom radial menu image '%s'.", L"Custom Menu Error", path.wstring().c_str());
		return {};
	}
}

CustomWheelsManager::CustomWheelsManager(std::vector<std::unique_ptr<Wheel>>& wheels, ImFont* font)
    : wheels_(wheels), font_(font)
{
}

void CustomWheelsManager::DrawOffscreen(ID3D11Device* dev, ID3D11DeviceContext* ctx)
{
	if(!loaded_)
		Reload(dev);
	else if(!textDraws_.empty())
	{
		auto& td = textDraws_.front();
		DrawText(ctx, td.rt, font_, td.size, td.text);
	    textDraws_.pop_front();
	}
}


void CustomWheelsManager::Draw(ID3D11DeviceContext* ctx)
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

std::unique_ptr<Wheel> CustomWheelsManager::BuildWheel(const std::filesystem::path& configPath, ID3D11Device* dev)
{
	cref dataFolder = configPath.parent_path();

	auto fail = [&](const wchar_t* error)
	{
		failedLoads_.push_back(error + (L": '" + configPath.wstring() + L"'"));
		return nullptr;
	};

	cref cfgSource = FileSystem::ReadFile(configPath);
	if(cfgSource.empty())
		return nullptr;

	CSimpleIniA ini(true);
	cref loadResult = ini.LoadData(reinterpret_cast<const char*>(cfgSource.data()), cfgSource.size());
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

	auto wheel = std::make_unique<Wheel>(IDR_BG, IDR_WIPEMASK, wheelNickname->second, wheelDisplayName->second, dev);
	wheel->outOfCombat_.enabled = ini.GetBoolValue("General", "only_out_of_combat", false);
	wheel->aboveWater_.enabled = ini.GetBoolValue("General", "only_above_water", false);

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
		cref elementPremultipliedAlpha = element.find("premultiply_alpha");

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
		ces.shadow = elementShadow == element.end() ? 1.f : float(atof(elementShadow->second));
		ces.colorize = elementColorize == element.end() ? 1.f : float(atof(elementColorize->second));
		ces.premultiply = false;

		if(elementIcon != element.end()) {
			ces.texture = LoadCustomTexture(dev, dataFolder / utf8_decode(elementIcon->second));
			if(elementPremultipliedAlpha != element.end())
				ces.premultiply = ini.GetBoolValue(sec.pItem, "premultiply_alpha", true);
		}

		if(!ces.texture.texture)
			maxTextWidth = std::max(maxTextWidth, CalcText(font_, utf8_decode(ces.name)));

		elements.push_back(ces);

		GW2_ASSERT(baseId < customWheelNextId_ + CustomWheelIdStep);
	}

	float desiredFontSize = 1024.f / maxTextWidth * 100.f;

	for(auto& ces : elements)
	{
	    if(!ces.texture.texture) {
		    ces.texture = MakeTextTexture(dev, desiredFontSize);
			textDraws_.push_back({ desiredFontSize, utf8_decode(ces.name), ces.texture });
		}

	    wheel->AddElement(std::make_unique<CustomElement>(ces, dev));
	}

	customWheelNextId_ += CustomWheelIdStep;

	return std::move(wheel);
}

void CustomWheelsManager::Reload(ID3D11Device* dev)
{
	{
	    APTTYPE a;
	    APTTYPEQUALIFIER b;
	    if(CoGetApartmentType(&a, &b) == CO_E_NOTINITIALIZED) {
	        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (hr != S_FALSE && hr != RPC_E_CHANGED_MODE && FAILED(hr))
	            CriticalMessageBox(L"Could not initialize COM library: error code 0x%X.", hr);
	    }
	}

	failedLoads_.clear();
	textDraws_.clear();
	customWheelNextId_ = CustomWheelStartId;

    if(!customWheels_.empty()) {
	    wheels_.erase(std::remove_if(wheels_.begin(), wheels_.end(), [&](cref ptr)
	    {
		    return std::find(customWheels_.begin(), customWheels_.end(), ptr.get()) != customWheels_.end();
	    }), wheels_.end());

		customWheels_.clear();
	}

	auto folderBaseOpt = ConfigurationFile::i().folder();
	if (folderBaseOpt)
	{
		auto folderBase = *folderBaseOpt / L"custom";

		if (std::filesystem::exists(folderBase))
		{
			auto addWheel = [&](const std::filesystem::path& configFile)
			{
				auto wheel = BuildWheel(configFile, dev);
				if (wheel)
				{
					wheels_.push_back(std::move(wheel));
					customWheels_.push_back(wheels_.back().get());
				}
			};

			for (cref entry : std::filesystem::directory_iterator(folderBase))
			{
				if (!entry.is_directory() && entry.path().extension() != L".zip")
					continue;

				std::filesystem::path configFile = entry.path() / L"config.ini";
				if (FileSystem::Exists(configFile))
					addWheel(configFile);
				else if (auto dirs = FileSystem::IterateZipFolders(entry.path()); !dirs.empty())
				{
					for (cref subdir : dirs)
					{
						std::filesystem::path subdirCfgFile = subdir / L"config.ini";
						if (FileSystem::Exists(subdirCfgFile))
							addWheel(subdirCfgFile);
					}
				}

			}
		}
	}

	loaded_ = true;
}

CustomElement::CustomElement(const CustomElementSettings& ces, ID3D11Device* dev)
	: WheelElement(ces.id, ces.nickname, ces.category, ces.name, dev, ces.texture), color_(ces.color)
{
    shadowStrength_ = ces.shadow;
	colorizeAmount_ = ces.colorize;
	premultiplyAlpha_ = ces.premultiply;
}

fVector4 CustomElement::color()
{
	return color_;
}

}
