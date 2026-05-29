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
    VECTOR m_position;
    VECTOR m_direction;
    LaserColor m_initalColor;
    int m_maxReflections;

    bool m_isHitObstacle;

    float m_currentLength;
    float m_growSpeed;

    bool m_isLaserStart;

    std::vector<RayHistory> m_history; // 反射の履歴リスト
    bool m_isLooping;

    bool CheckLineIntersection(VECTOR p1, VECTOR p2, VECTOR p3, VECTOR p4, VECTOR* outIntersection);

    float GetDistanceLineToPoint(VECTOR p1, VECTOR p2, VECTOR pt);

public:
    // コンストラクタ
    Laser(VECTOR position, VECTOR direction, LaserColor initalColor, int maxReflections = 100, float growSpeed = 6.0f); // 最大回数を100などに増やしても安全になります
    ~Laser() {}

    void Reset();
    void Update();
    void Draw(const std::vector<Mirror>& mirrors, const std::vector<Obstacle>& obstacles, const std::vector<Filter>& filters, LaserTarget& target);

    VECTOR GetEndPoint() const { return m_history.back().position; }
    bool IsHitObstacle() const { return m_isHitObstacle; }
    auto GetHistory() const { return m_history; };
    bool IsLoop() const { return m_isLooping; }
};