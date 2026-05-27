#pragma once
#include "DxLib.h"

// --- Target.h の冒頭 ---
enum class LaserColor : unsigned int {
	Red = 0,
	Green = 1,
	Blue = 2
};

class LaserTarget {
private:
	VECTOR m_position;
	float m_radius;
	bool m_isHit;
	LaserColor m_requiredColor; // ★追加：要求する色

public:
	// コンストラクタ（位置とサイズを指定）
	LaserTarget(VECTOR position, float radius = 25.0f, LaserColor requiredColor = LaserColor::Red);
	~LaserTarget() {}

	// 毎フレームの最初に、一旦当たり判定をリセットする
	void ResetHitState();

	// レーザーから「当たったよ」と通知してもらう関数
	void SetHit(LaserColor laserColor, bool hasFilter);

	// 描画処理：状態（m_isHit）によって色や見た目を変える
	void Draw(bool hasFilter = false);

	// ゲッター
	VECTOR GetPosition()const { return m_position; }
	float GetRadius()	const { return m_radius; }
	bool IsHit()		const { return m_isHit; }
};