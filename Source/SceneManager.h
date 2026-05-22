#pragma once

#pragma once
#include "Scene.h"

class SceneManager {
private:
    Scene* m_currentScene; // 現在実行中のシーンへのポインタ

    // シーンを新しく生成するためのヘルパー関数
    void ChangeScene(SceneName nextScene);

public:
    SceneManager();
    ~SceneManager();

    void Initialize(); // 最初のシーン（タイトル）を設定
    void Update();     // 現在のシーンの更新と、シーン遷移のチェック
    void Draw();       // 現在のシーンの描画
};