#pragma once
#include "Scene.h"

class TitleScene : public Scene {
    // --- TitleScene.h の private に追加 ---
private:
    int m_flashTimer;
    float m_introFade; // 💡 【追加】起動時のじわっとフェードイン用タイマー

    // 💡 フォントハンドルを保持する変数
    int m_fontLarge;  // タイトルロゴ用
    int m_fontMedium; // スタート合図用
    int m_fontSmall;  // サブタイトル・クレジット用

public:
    TitleScene();
    virtual ~TitleScene(); // 💡 デストラクタでフォントを解放するために変更

    virtual void Initialize() override;
    virtual SceneName Update() override;
    virtual void Draw() override;
};