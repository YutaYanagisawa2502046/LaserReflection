#pragma once
#include "DxLib.h"
#include <vector>

class Mirror;
class LaserTarget;
class Obstacle; // 前方宣言

// 軌道の履歴を記録するための構造体
struct RayHistory {
    VECTOR position;  // 反射した位置（始点）
    VECTOR direction; // そのときの進行方向
};

class Laser {
private:
    VECTOR m_position;
    VECTOR m_direction;
    int m_maxReflections;

    float m_currentLength;
    float m_growSpeed;

    // ---- 無限ループ判定用の追加変数 ----
    std::vector<RayHistory> m_history; // 反射の履歴リスト
    bool m_isLooping;                  // 無限ループを検知したかどうかのフラグ

    bool CheckLineIntersection(VECTOR p1, VECTOR p2, VECTOR p3, VECTOR p4, VECTOR* outIntersection);

    float GetDistanceLineToPoint(VECTOR p1, VECTOR p2, VECTOR pt);

    // 過去に同じ軌道を通ったかチェックする判定関数
    bool IsDuplicateOrbit(VECTOR pos, VECTOR dir);

public:
    // コンストラクタ
    Laser(VECTOR position, VECTOR direction, int maxReflections = 100, float growSpeed = 6.0f); // 最大回数を100などに増やしても安全になります
    ~Laser() {}

    void Reset();
    void Update();
    void Draw(const std::vector<Mirror>& mirrors, const std::vector<Obstacle>& obstacles, LaserTarget& target);
};