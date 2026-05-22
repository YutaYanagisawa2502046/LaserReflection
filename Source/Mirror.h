#pragma once
#include "DxLib.h"

class Mirror {
private:
    VECTOR m_start;  // 反射板の始点座標 (x, y)
    VECTOR m_end;    // 反射板の終点座標 (x, y)
    VECTOR m_normal; // 反射板の面法線ベクトル（垂直な方向、向きを決定する）

    // 始点と終点から法線ベクトルを自動計算する内部関数
    void CalculateNormal();

public:
    // コンストラクタ（始点と終点を指定して生成）
    Mirror(VECTOR start, VECTOR end);
    ~Mirror() {}

    void Draw();     // 反射板を描画する

    // 外部から座標や法線を取得するための関数（ゲッター）
    VECTOR GetStart()  const { return m_start; }
    VECTOR GetEnd()    const { return m_end; }
    VECTOR GetNormal() const { return m_normal; }
};