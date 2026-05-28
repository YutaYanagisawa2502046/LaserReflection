
#pragma once
#include "DxLib.h"

class Obstacle {
private:
    VECTOR m_start; // 壁の始点
    VECTOR m_end;   // 壁の終点

public:
    Obstacle(VECTOR start, VECTOR end) : m_start(start), m_end(end) {}
    ~Obstacle() {}

    void Draw() const {
        // 吸収する黒～不気味な紫色の分厚い壁を描画
        // ちょっと太め（太さ 5）にして存在感を出します
        DrawLine((int)m_start.x, (int)m_start.y, (int)m_end.x, (int)m_end.y, GetColor(180, 50, 50), 5);
        // 内側に黒い線を重ねて「コア」を表現
        DrawLine((int)m_start.x, (int)m_start.y, (int)m_end.x, (int)m_end.y, GetColor(20, 20, 20), 1);
    }

    // ゲッター
    VECTOR GetStart() const { return m_start; }
    VECTOR GetEnd()   const { return m_end; }
    VECTOR GetNormal()const { VECTOR tmp = VSub(m_start, m_end); return VGet(tmp.y, tmp.x, 0.f); }
};