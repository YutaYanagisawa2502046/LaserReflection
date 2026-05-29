#pragma once
#include <vector>
#include <string>

enum class SE
{
	LASER,
	WALLHIT,
	BOMB
};
class SoundManager
{
public:
	SoundManager();
	~SoundManager();

	void AddSEData(std::string fileName, SE seType);

	void PlaySE(SE se, bool isLoop = false);
	void StopSE(SE se);

	bool CheckPlaySE(SE se);

	int SerchSEHandle(SE se);

private:
	std::vector<std::pair<int, SE>>m_seMap;
};