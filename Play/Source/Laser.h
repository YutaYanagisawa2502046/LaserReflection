#pragma once
#include "DxLib.h"
#include <vector>
#include "LaserTarget.h"

class Mirror;
class Obstacle; // 前方宣言
class Filter;

// 軌道の履歴を記録するための構造体
struct RayHistory {
    VECTOR position;  // 反射した位置（始点）
    VECTOR direction; // そのときの進行方向
    LaserColor Color;

    RayHistory() = default;
};

class Laser {
private:
	VECTOR m_position;  // レーザーの発射位置
	VECTOR m_direction; // レーザーの初期進行方向（単位ベクトルで正規化されていることが前提）
	LaserColor m_initalColor;   // レーザーの初期色
	int m_maxReflections;       // レーザーが反射できる最大回数（安全のために100などに増やしても良い）

	bool m_isHitObstacle;       // レーザーが壁に当たっているかのフラグ

	float m_currentLength;  // 現在のレーザーの可視長さ（0から徐々に伸びていく）
	float m_growSpeed;      // レーザーの長さが伸びる速度（フレームごとにどれだけ伸びるか）

	bool m_isLaserStart;    // レーザーが最初に発射されたかどうかのフラグ（SE再生のため）

    std::vector<RayHistory> m_history; // 反射の履歴リスト
	bool m_isLooping;   // 無限ループを検知したかどうかのフラグ

	// 交差判定関数
    bool CheckLineIntersection(VECTOR p1, VECTOR p2, VECTOR p3, VECTOR p4, VECTOR* outIntersection);

	// 線分と点の最短距離を求める関数
    float GetDistanceLineToPoint(VECTOR p1, VECTOR p2, VECTOR pt);

public:
    // コンストラクタ
    Laser(VECTOR position, VECTOR direction, LaserColor initalColor, int maxReflections = 100, float growSpeed = 6.0f); // 最大回数を100などに増やしても安全になります
    ~Laser() {}

	void Reset();   // レーザーの状態を初期化する関数（再発射のときなどに呼び出す）
    void Update();
	// レーザーの軌道を計算して描画する関数。鏡や壁、フィルターとの衝突判定もここで行います。
    void Draw(const std::vector<Mirror>& mirrors, const std::vector<Obstacle>& obstacles, const std::vector<Filter>& filters, LaserTarget& target);

	VECTOR GetEndPoint() const { return m_history.back().position; }    // レーザーの現在の先端座標を取得する関数
	bool IsHitObstacle() const { return m_isHitObstacle; }              // レーザーが壁に当たっているかのフラグを取得する関数
	auto GetHistory() const { return m_history; };						// レーザーの軌道履歴を取得する関数
	bool IsLoop() const { return m_isLooping; }							// レーザーが無限ループを検知しているかのフラグを取得する関数
	VECTOR GetPosition() const { return m_position; } // レーザーの発射位置を取得する関数
	VECTOR GetDirection() const { return m_direction; } // レーザーの進行方向を取得する関数
	LaserColor GetInitialColor() const { return m_initalColor; } // レーザーの初期色を取得する関数
};