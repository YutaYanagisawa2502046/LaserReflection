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

	// SEデータを追加する関数。ファイル名とSEの種類を指定して呼び出します。
	void AddSEData(std::string fileName, SE seType);

	// SEを再生する関数。SEの種類と、ループ再生するかどうかを指定して呼び出します。
	void PlaySE(SE se, bool isLoop = false);
	// SEを停止する関数。SEの種類を指定して呼び出します。
	void StopSE(SE se);
	// SEが現在再生中かどうかをチェックする関数。SEの種類を指定して呼び出します。
	bool CheckPlaySE(SE se);
	// SEの種類から、内部で管理しているサウンドハンドルを検索して返す関数。見つからない場合は -1 を返します。
	int SerchSEHandle(SE se);

private:
	std::vector<std::pair<int, SE>>m_seMap;	// SEの種類とサウンドハンドルをペアで管理するリスト
};