#include <Keybind.h>
#include <Utility.h>
#include <sstream>
#include <ConfigurationFile.h>
#include <numeric>

namespace GW2Radial
{
std::unordered_map<Keybind*, std::set<ScanCode>> Keybind::scMaps_;

Keybind::Keybind(std::string nickname, std::string displayName, const std::set<ScanCode>& scs, bool saveToConfig) :
	nickname_(std::move(nickname)), displayName_(std::move(displayName)), saveToConfig_(saveToConfig)
{
	this->scanCodes(scs);
	isBeingModified_ = false;
}

Keybind::Keybind(std::string nickname, std::string displayName) :
	nickname_(std::move(nickname)), displayName_(std::move(displayName))
{
	const auto keys = ConfigurationFile::i()->ini().GetValue("Keybinds", nickname_.c_str());
	if(keys) this->scanCodes(keys);
	isBeingModified_ = false;
}

Keybind::~Keybind()
{
	scMaps_.erase(this);
}

void Keybind::scanCodes(const std::set<ScanCode>& scs)
{
	if(!isBeingModified_)
		return;

	scanCodes_ = scs;

	ApplyKeys();
}

void Keybind::scanCodes(const char* keys)
{
	if(!isBeingModified_)
		return;

	scanCodes_.clear();

	if (strnlen_s(keys, 256) > 0)
	{
		std::stringstream ss(keys);
		std::vector<std::string> result;

		while (ss.good())
		{
			std::string substr;
			std::getline(ss, substr, ',');
			const auto val = std::stoi(substr);
			scanCodes_.insert(static_cast<ScanCode>(val));
		}
	}

	ApplyKeys();
}

void Keybind::ApplyKeys()
{
	UpdateDisplayString();

	CheckForConflict();
	
	if(saveToConfig_)
	{
		std::string settingValue;
		for (const auto& k : scanCodes_)
			settingValue += std::to_string(uint(k)) + ", ";

		if(!scanCodes_.empty())
			settingValue = settingValue.substr(0, settingValue.size() - 2);

		auto cfg = ConfigurationFile::i();
		cfg->ini().SetValue("keybinds", nickname_.c_str(), settingValue.c_str());
		cfg->Save();
	}
}

void Keybind::CheckForConflict(bool recurse)
{
	scMaps_[this] = scanCodes_;
	
	isConflicted_ = false;
	for(auto& elem : scMaps_)
	{
		if(elem.first != this && !scanCodes_.empty() && elem.second == scanCodes_)
			isConflicted_ = true;
		
		if(recurse) elem.first->CheckForConflict(false);
	}
}

bool Keybind::conflicts(const std::set<ScanCode>& scanCodes) const
{
	for(auto& elem : scMaps_)
	{
		if(elem.first != this && scanCodes_.size() < elem.second.size()
			&& std::includes(scanCodes.begin(), scanCodes.end(), elem.second.begin(), elem.second.end()))
			return true;
	}

	return false;
}

bool Keybind::matchesNoLeftRight(const std::set<ScanCode>& scanCodes) const
{
	auto k2 = scanCodes;
	const auto replace = [&](ScanCode sc, ScanCode replacement)
	{
		if(const auto k = k2.find(sc); k != k2.end())
		{
			k2.erase(k);
			k2.insert(replacement);
		}
	};
	replace(ScanCode::CONTROLRIGHT, ScanCode::CONTROLLEFT);
	replace(ScanCode::SHIFTRIGHT, ScanCode::SHIFTLEFT);
	replace(ScanCode::ALTRIGHT, ScanCode::ALTLEFT);

	return k2 == scanCodes_;
}

void Keybind::UpdateDisplayString()
{
	if(scanCodes_.empty())
	{
		keysDisplayString_[0] = '\0';
		return;
	}

	std::wstring keybind = std::accumulate(std::next(scanCodes_.begin()), scanCodes_.end(), GetScanCodeName(*scanCodes_.begin()), [](std::wstring a, ScanCode b) { return std::move(a) + L" + " + GetScanCodeName(b); });

	strcpy_s(keysDisplayString_.data(), keysDisplayString_.size(), utf8_encode(keybind).c_str());
}

}
