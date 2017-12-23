/*
* The MIT License
* Copyright 2017 naPalm / PapaRadroach
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute,
* sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
* LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
* THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include "stdafx.h"

typedef union
{
	float valFloat;
	int valInt;
	bool valBool;
} ConfigValueU;

class ConfigurationManager
{
private:
	char* pszINIFilePath;

	ConfigValueU getValue(std::string key)
	{
		ConfigValueU result;
		result.valBool = 0;
		result.valInt = 0;
		result.valFloat = 0;

		if(m_ConfigValues.find(key) != m_ConfigValues.end())
		{
			result = m_ConfigValues[key];
		}

		return result;
	}

public:
	std::map<std::string, ConfigValueU> m_ConfigValues;

	ConfigurationManager(char* INIFilePath)
	{
		this->pszINIFilePath = INIFilePath;
	}

	bool Init()
	{
		m_ConfigValues["bPlaceInRed"].valBool = true;
		m_ConfigValues["bDisableObjectSnap"].valBool = false;
		m_ConfigValues["bDisableGroundSnap"].valBool = false;
		m_ConfigValues["bDisableObjectHighlighting"].valBool = false;
		m_ConfigValues["fObjectZoomSpeed"].valFloat = 10.0f;
		m_ConfigValues["fObjectRotationSpeed"].valFloat = 5.0f;
		m_ConfigValues["bEnableAchievementsModded"].valBool = false;

		CSimpleIniA ini;
		ini.SetUnicode();
		SI_Error result = ini.LoadFile(pszINIFilePath);
		CSimpleIniA::TNamesDepend sections;
		if(result >= 0)
		{
			ini.GetAllSections(sections);
		}

		if(sections.empty())
		{
			result = ini.SetValue("Workshop", NULL, NULL);
			if(result < 0)
			{
				MessageBoxA(NULL, "INI: Failed to create section Workshop", TITLE, MB_ICONERROR);
				return false;
			}

			result = ini.SetValue("Misc", NULL, NULL);
			if(result < 0)
			{
				MessageBoxA(NULL, "INI: Failed to create section Misc", TITLE, MB_ICONERROR);
				return false;
			}

			std::string sDescription = ";Enable Place in Red (place workshop objects anywhere)> Default: " + std::to_string(m_ConfigValues["bPlaceInRed"].valBool);
			result = ini.SetValue("Workshop", "bPlaceInRed", std::to_string(m_ConfigValues["bPlaceInRed"].valBool).c_str(), sDescription.c_str());
			if(result < 0)
			{
				MessageBoxA(NULL, "INI: Failed to create key bPlaceInRed", TITLE, MB_ICONERROR);
				return false;
			}

			sDescription = ";Disable snapping between objects? Default: " + std::to_string(m_ConfigValues["bDisableObjectSnap"].valBool);
			result = ini.SetValue("Workshop", "bDisableObjectSnap", std::to_string(m_ConfigValues["bDisableObjectSnap"].valBool).c_str(), sDescription.c_str());
			if(result < 0)
			{
				MessageBoxA(NULL, "INI: Failed to create key bDisableObjectSnap", TITLE, MB_ICONERROR);
				return false;
			}

			sDescription = ";Disable ground snapping for objects? Default: " + std::to_string(m_ConfigValues["bDisableGroundSnap"].valBool);
			result = ini.SetValue("Workshop", "bDisableGroundSnap", std::to_string(m_ConfigValues["bDisableGroundSnap"].valBool).c_str(), sDescription.c_str());
			if(result < 0)
			{
				MessageBoxA(NULL, "INI: Failed to create key bDisableGroundSnap", TITLE, MB_ICONERROR);
				return false;
			}

			sDescription = ";Disable object highlighting while in workshop mode? Default: " + std::to_string(m_ConfigValues["bDisableObjectHighlighting"].valBool);
			result = ini.SetValue("Workshop", "bDisableObjectHighlighting", std::to_string(m_ConfigValues["bDisableObjectHighlighting"].valBool).c_str(), sDescription.c_str());
			if(result < 0)
			{
				MessageBoxA(NULL, "INI: Failed to create key bDisableObjectHighlighting", TITLE, MB_ICONERROR);
				return false;
			}

			sDescription = ";Set the object zoom speed. Default: " + std::to_string(m_ConfigValues["fObjectZoomSpeed"].valFloat);
			result = ini.SetValue("Workshop", "fObjectZoomSpeed", std::to_string(m_ConfigValues["fObjectZoomSpeed"].valFloat).c_str(), sDescription.c_str());
			if(result < 0)
			{
				MessageBoxA(NULL, "INI: Failed to create key fObjectZoomSpeed", TITLE, MB_ICONERROR);
				return false;
			}

			sDescription = ";Set the object rotation speed. Default: " + std::to_string(m_ConfigValues["fObjectRotationSpeed"].valFloat);
			result = ini.SetValue("Workshop", "fObjectRotationSpeed", std::to_string(m_ConfigValues["fObjectRotationSpeed"].valFloat).c_str(), sDescription.c_str());
			if(result < 0)
			{
				MessageBoxA(NULL, "INI: Failed to create key fObjectRotationSpeed", TITLE, MB_ICONERROR);
				return false;
			}

			sDescription = ";Enable achievements in modded games? Default: " + std::to_string(m_ConfigValues["bEnableAchievementsModded"].valBool);
			result = ini.SetValue("Misc", "bEnableAchievementsModded", std::to_string(m_ConfigValues["bEnableAchievementsModded"].valBool).c_str(), sDescription.c_str());
			if(result < 0)
			{
				MessageBoxA(NULL, "INI: Failed to create key bEnableAchievementsModded", TITLE, MB_ICONERROR);
				return false;
			}

			result = ini.SaveFile(INI_PATH);
			if(result < 0)
			{
				MessageBoxA(NULL, "INI: Failed to create the ini file", TITLE, MB_ICONERROR);
				return false;
			}
		}
		else
		{
			m_ConfigValues["bPlaceInRed"].valBool = atoi(ini.GetValue("Workshop", "bPlaceInRed", std::to_string(m_ConfigValues["bPlaceInRed"].valBool).c_str())) > 0;
			m_ConfigValues["bDisableObjectSnap"].valBool = atoi(ini.GetValue("Workshop", "bDisableObjectSnap", std::to_string(m_ConfigValues["bDisableObjectSnap"].valBool).c_str())) > 0;
			m_ConfigValues["bDisableGroundSnap"].valBool = atoi(ini.GetValue("Workshop", "bDisableGroundSnap", std::to_string(m_ConfigValues["bDisableGroundSnap"].valBool).c_str())) > 0;
			m_ConfigValues["bDisableObjectHighlighting"].valBool = atoi(ini.GetValue("Workshop", "bDisableObjectHighlighting", std::to_string(m_ConfigValues["bDisableObjectHighlighting"].valBool).c_str())) > 0;
			m_ConfigValues["fObjectZoomSpeed"].valFloat = strtof(ini.GetValue("Workshop", "fObjectZoomSpeed", std::to_string(m_ConfigValues["fObjectZoomSpeed"].valFloat).c_str()), NULL);
			m_ConfigValues["fObjectRotationSpeed"].valFloat = strtof(ini.GetValue("Workshop", "fObjectRotationSpeed", std::to_string(m_ConfigValues["fObjectRotationSpeed"].valFloat).c_str()), NULL);
			m_ConfigValues["bEnableAchievementsModded"].valBool = atoi(ini.GetValue("Misc", "bEnableAchievementsModded", std::to_string(m_ConfigValues["bEnableAchievementsModded"].valBool).c_str())) > 0;
		}

		return true;
	}

	bool SaveSetting(const char* section, const char* key, const char* value)
	{
		CSimpleIniA ini;
		ini.SetUnicode();

		SI_Error result = ini.LoadFile(pszINIFilePath);
		if(result < 0)
		{
			return false;
		}

		result = ini.SetValue(section, key, value);
		if(result < 0)
		{
			return false;
		}

		ini.SaveFile(INI_PATH);
		if(result < 0)
		{
			return false;
		}

		m_ConfigValues[std::string(key)].valBool = atoi(value) > 0;
		m_ConfigValues[std::string(key)].valInt = atoi(value);
		m_ConfigValues[std::string(key)].valFloat = strtof(value, NULL);

		return true;
	}

	bool GetBool(std::string key)
	{
		return getValue(key).valBool;
	}

	int GetInt(std::string key)
	{
		return getValue(key).valInt;
	}

	float GetFloat(std::string key)
	{
		return getValue(key).valFloat;
	}
};