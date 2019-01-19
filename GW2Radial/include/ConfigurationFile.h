#pragma once
#include <Main.h>
#include <simpleini/SimpleIni.h>
#include <Singleton.h>

namespace GW2Radial
{

class ConfigurationFile : public Singleton<ConfigurationFile>
{
public:
	ConfigurationFile();

	void Reload();
	void Save();
	
	const std::string& lastSaveError() const { return lastSaveError_; }
	bool lastSaveErrorChanged() const { return lastSaveErrorChanged_; }
	void lastSaveErrorChanged(bool v) { lastSaveErrorChanged_ = v; lastSaveError_.clear(); }

	const char* imguiLocation() const { return imguiLocation_; }
	const TCHAR* location() const { return location_; }

	CSimpleIniA& ini() { return ini_; }

protected:

	CSimpleIniA ini_;
	tstring folder_;
	TCHAR location_[MAX_PATH] { };
	char imguiLocation_[MAX_PATH] { };

	bool lastSaveErrorChanged_ = false;
	std::string lastSaveError_;
};

}
