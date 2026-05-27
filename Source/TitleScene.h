#pragma once
#include "Scene.h"

class TitleScene : public Scene {
private:
    int m_flashTimer; // 文字を点滅させるためのタイマー

public:
    TitleScene();
    virtual ~TitleScene() {};

    virtual void Initialize() override;
    virtual SceneName Update() override; // 💡 スペースキーが押されたら GameScene へ進む
    virtual void Draw() override;        // 💡 カッコいいロゴと「PRESS SPACE」を描画
};