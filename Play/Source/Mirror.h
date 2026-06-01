#pragma once
#include "DxLib.h"

class Mirror {
private:
    VECTOR m_start;  // 反射板の始点座標 (x, y)
    VECTOR m_end;    // 反射板の終点座標 (x, y)
    VECTOR m_normal; // 反射板の面法線ベクトル（垂直な方向、向きを決定する）

	bool m_isSelected;  // 鏡が選択されているかどうかのフラグ
	bool m_isChange;    // 鏡の角度が変更されたかどうかのフラグ
	float m_angle;      // 鏡の現在の角度（ラジアン）

    // 始点と終点から法線ベクトルを自動計算する内部関数
    void CalculateNormal();

public:
    // コンストラクタ（始点と終点を指定して生成）
    Mirror(VECTOR start, VECTOR end);
    ~Mirror() {}

    void Draw();     // 反射板を描画する
    void Update();

	void SetSelect(bool flag) { m_isSelected = flag; }  // 鏡が選択されているかどうかを設定する関数
	bool GetSelect() const { return m_isSelected; }     // 鏡が選択されているかどうかを取得する関数

	bool GetIsChange() const { return m_isChange; }     // 鏡の角度が変更されたかどうかを取得する関数

    // 外部から座標や法線を取得するための関数（ゲッター）
    VECTOR GetStart()  const { return m_start; }   // 反射板の始点座標を取得する関数
    VECTOR GetEnd()    const { return m_end; }     // 反射板の終点座標を取得する関数
    VECTOR GetNormal() const { return m_normal; }  // 反射板の法線ベクトルを取得する関数
};