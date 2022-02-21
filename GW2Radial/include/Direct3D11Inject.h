﻿#pragma once

#include <Main.h>
#include <xxhash/xxhash.h>
#include <d3d11.h>
#include <functional>
#include <Singleton.h>

namespace GW2Radial
{

class Direct3D11Inject : public Singleton<Direct3D11Inject, false>
{
public:
	using PrePresentSwapChainCallback = std::function<void()>;
	using PostCreateSwapChainCallback = std::function<void(HWND, ID3D11Device*, IDXGISwapChain*)>;

	PrePresentSwapChainCallback prePresentSwapChainCallback;
	PostCreateSwapChainCallback postCreateSwapChainCallback;

protected:
	Direct3D11Inject() = default;

	bool isFrameDrawn_ = false;
	bool isDirect3DHooked_ = false;
};

}
