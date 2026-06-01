#pragma once
#include "SoundManager.h"

// ゲーム全体で共有するマスターデータやシングルトン的な機能をまとめるクラス
class Master
{
public:
	static SoundManager* m_soundManager; // サウンドマネージャーポインタ

private:
	
};

// 便利なエイリアス（Master::m_soundManager を mst::m_soundManager と書けるようにする）
typedef Master mst;