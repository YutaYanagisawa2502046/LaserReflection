#pragma once
#include "Scene.h"

class TitleScene : public Scene {
private:
    int m_flashTimer;

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