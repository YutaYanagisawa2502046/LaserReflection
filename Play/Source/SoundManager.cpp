#include "SoundManager.h"
#include "DxLib.h"

SoundManager::SoundManager()
{
}

SoundManager::~SoundManager()
{
	// 登録されているSEのサウンドハンドルをすべて削除してリソースを解放します
	for (size_t i = 0; i < m_seMap.size(); i++)
	{
		DeleteSoundMem(m_seMap[i].first);
	}
}

void SoundManager::AddSEData(std::string fileName, SE seType)
{
	// すでに同じSEの種類が登録されているかをチェックします。重複して登録しないようにするためです。
	if (SerchSEHandle(seType) != -1)
	{
		return;
	}

	// ファイル名からサウンドハンドルをロードします。失敗した場合は -1 が返るので、その場合は登録を中止します。
	int handle = LoadSoundMem(fileName.c_str());
	if (handle == -1)
	{
		return;
	}

	// 成功した場合は、SEの種類とサウンドハンドルをペアでリストに追加して管理します。
	m_seMap.emplace_back(std::make_pair(handle, seType));
}

void SoundManager::PlaySE(SE se, bool isLoop)
{
	// SEの種類からサウンドハンドルを検索します。見つからない場合は -1 が返るので、その場合は再生を中止します。
	int handle = SerchSEHandle(se);

	// ループ再生するかどうかを指定して、サウンドハンドルを再生します。ループ再生の場合は DX_PLAYTYPE_LOOP を、そうでない場合は DX_PLAYTYPE_BACK を渡します。
	PlaySoundMem(handle, isLoop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK);
}

void SoundManager::StopSE(SE se)
{
	// SEの種類からサウンドハンドルを検索します。見つからない場合は -1 が返るので、その場合は停止を中止します。
	int handle = SerchSEHandle(se);

	// サウンドハンドルの再生を停止します。次回のループ終了のタイミングで音を止める場合は、第2引数に TRUE を渡します。
	StopSoundMem(handle);
}

bool SoundManager::CheckPlaySE(SE se)
{
	// SEの種類からサウンドハンドルを検索します。見つからない場合は -1 が返るので、その場合は再生中でないと判断して false を返します。
	int handle = SerchSEHandle(se);

	// サウンドハンドルが再生中かどうかを取得します。再生中の場合は 1 が返るので、その場合は true を返します。
	return (CheckSoundMem(handle) == 1);
}

int SoundManager::SerchSEHandle(SE se)
{
	// SEの種類とサウンドハンドルをペアで管理しているリストを順番に検索して、指定されたSEの種類に対応するサウンドハンドルを見つけます。見つからない場合は -1 を返します。
	for (size_t i = 0; i < m_seMap.size(); i++)
	{
		// SEの種類が一致するかをチェックします。もし一致する場合は、そのサウンドハンドルを返します。
		if (m_seMap[i].second == se)
		{
			return m_seMap[i].first;
		}
	}
	// もしリストの最後まで検索しても見つからない場合は、-1 を返します。
	return -1;
}
