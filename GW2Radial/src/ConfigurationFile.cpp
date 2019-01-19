#include <ConfigurationFile.h>
#include <Shlobj.h>
#include <Utility.h>
#include <tchar.h>
#include <sstream>

namespace GW2Radial
{
DEFINE_SINGLETON(ConfigurationFile);
	
const TCHAR* g_configName = TEXT("config.ini");
const TCHAR* g_imguiConfigName = TEXT("imgui_config.ini");

ConfigurationFile::ConfigurationFile()
{
	Reload();
}

void ConfigurationFile::Reload()
{
	// Create folders
	TCHAR exeFullPath[MAX_PATH];
	GetModuleFileName(nullptr, exeFullPath, MAX_PATH);
	tstring exeFolder;
	SplitFilename(exeFullPath, &exeFolder, nullptr);
	folder_ = exeFolder + TEXT("\\addons\\gw2radial\\");
	_tcscpy_s(location_, (folder_ + g_configName).c_str());
#if _UNICODE
	strcpy_s(imguiLocation_, utf8_encode(folder_ + g_imguiConfigName).c_str());
#else
	strcpy_s(_ImGuiConfigLocation, (_ConfigFolder + ImGuiConfigName).c_str());
#endif
	SHCreateDirectoryEx(nullptr, folder_.c_str(), nullptr);

	// Load INI settings
	ini_.SetUnicode();
	ini_.LoadFile(location_);
}

void ConfigurationFile::Save()
{
	const auto r = ini_.SaveFile(location_);

	if (r < 0)
	{
		const auto prevSaveError = lastSaveError_;
		switch (r)
		{
		case SI_FAIL:
			lastSaveError_ = "Unknown error";
			break;
		case SI_NOMEM:
			lastSaveError_ = "Out of memory";
			break;
		case SI_FILE:
			char buf[1024];
			if (strerror_s(buf, errno) == 0)
				lastSaveError_ = buf;
			else
				lastSaveError_ = "Unknown error";
			break;
		default:
			lastSaveError_ = "Unknown error";
		}

		lastSaveErrorChanged_ |= prevSaveError != lastSaveError_;
	}
	else if(!lastSaveError_.empty())
	{
		lastSaveError_.clear();
		lastSaveErrorChanged_ = true;
	}
}

}