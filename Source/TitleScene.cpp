#include "TitleScene.h"
#include "DxLib.h"

void TitleScene::Initialize() {
    // タイトルシーン独自の初期化（画像や音の読み込みなど）があればここで行います
}

SceneName TitleScene::Update() {
    // エンターキーが押されたら、ゲームシーンの名前を返して遷移を促す
    if (CheckHitKey(KEY_INPUT_RETURN) == 1) {
        return SceneName::Game;
    }

    // キーが押されていなければ遷移しない
    return SceneName::None;
}

void TitleScene::Draw() {
    // 画面中央付近にテキストを表示（フォントサイズや色はデフォルト）
    DrawString(200, 200, "レーザー反射ゲーム - タイトル画面", GetColor(255, 255, 255));
    DrawString(220, 260, "[ENTER] キーを押してスタート", GetColor(200, 200, 200));
}