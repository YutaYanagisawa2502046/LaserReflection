#include "SoundManager.h"
#include "DxLib.h"

SoundManager::SoundManager()
{
}

SoundManager::~SoundManager()
{
	for (size_t i = 0; i < m_seMap.size(); i++)
	{
		DeleteSoundMem(m_seMap[i].first);
	}
}

void SoundManager::AddSEData(std::string fileName, SE seType)
{
	if (SerchSEHandle(seType) != -1)
	{
		return;
	}

	int handle = LoadSoundMem(fileName.c_str());
	if (handle == -1)
	{
		return;
	}

	m_seMap.emplace_back(std::make_pair(handle, seType));
}

void SoundManager::PlaySE(SE se, bool isLoop)
{
	int handle = SerchSEHandle(se);

	PlaySoundMem(handle, isLoop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK);
}

void SoundManager::StopSE(SE se)
{
	int handle = SerchSEHandle(se);

	StopSoundMem(handle);
}

bool SoundManager::CheckPlaySE(SE se)
{
	int handle = SerchSEHandle(se);

	return (CheckSoundMem(handle) == 1);
}

int SoundManager::SerchSEHandle(SE se)
{
	for (size_t i = 0; i < m_seMap.size(); i++)
	{
		if (m_seMap[i].second == se)
		{
			return m_seMap[i].first;
		}
	}
	return -1;
}
