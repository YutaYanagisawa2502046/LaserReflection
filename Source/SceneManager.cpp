#include "SceneManager.h"
#include "TitleScene.h"
#include "GameScene.h"

SceneManager::SceneManager() : m_currentScene(nullptr) {}

SceneManager::~SceneManager() {
    // メモリリーク防止のため、終了時に現在のシーンを削除
    if (m_currentScene != nullptr) {
        delete m_currentScene;
    }
}

void SceneManager::Initialize() {
    // 起動時は「タイトルシーン」からスタート
    ChangeScene(SceneName::Title);
}

void SceneManager::Update() {
    if (m_currentScene == nullptr) return;

    // 現在のシーンを更新し、次のシーンの要望を受け取る
    SceneName next = m_currentScene->Update();

    // 次のシーンへの遷移要求（None以外）があれば切り替える
    if (next != SceneName::None) {
        ChangeScene(next);
    }
}

void SceneManager::Draw() {
    if (m_currentScene == nullptr) return;

    // 現在のシーンの描画処理を呼ぶ
    m_currentScene->Draw();
}

// シーン切り替えの実体
void SceneManager::ChangeScene(SceneName nextScene) {
    // 1. 今動いているシーンを片付ける
    if (m_currentScene != nullptr) {
        delete m_currentScene;
        m_currentScene = nullptr;
    }

    // 2. 指定された新しいシーンを生成する
    switch (nextScene) {
    case SceneName::Title:
        m_currentScene = new TitleScene();
        break;
    case SceneName::Game:
        m_currentScene = new GameScene();
        break;
    default:
        return;
    }

    // 3. 新しいシーンの初期化を行う
    m_currentScene->Initialize();
}