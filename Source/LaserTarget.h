#pragma once
#include "DxLib.h"

class LaserTarget {
private:
    VECTOR m_position;  // 的の中心座標 (x, y)
    float m_radius;     // 的の半径
    bool m_isHit;       // 今このフレームでレーザーが当たっているかどうかのフラグ

public:
    // コンストラクタ（位置とサイズを指定）
    LaserTarget(VECTOR position, float radius = 25.0f);
    ~LaserTarget() {}

    // 毎フレームの最初に、一旦当たり判定をリセットする
    void ResetHitState();

    // レーザーから「当たったよ」と通知してもらう関数
    void SetHit();

    // 描画処理：状態（m_isHit）によって色や見た目を変える
    void Draw();

    // ゲッター
    VECTOR GetPosition() const { return m_position; }
    float GetRadius()     const { return m_radius; }
    bool IsHit()          const { return m_isHit; }
};