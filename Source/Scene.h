#pragma once

// シーンの名前を管理するための列挙型（enum）
enum class SceneName {
    Title,  // タイトルシーン
    Game,   // ゲーム本編シーン
    None    // シーン遷移を行わないときに使用
};

// すべてのシーンの基底となるクラス
class Scene {
public:
    virtual ~Scene() {}

    // 初期化処理：シーンが始まったときに1度だけ呼ばれる
    virtual void Initialize() = 0;

    // 更新処理：毎フレーム呼ばれる（計算や入力の受付）
    // 戻り値：次のフレームで遷移したいシーンの名前（遷移しない場合は SceneName::None）
    virtual SceneName Update() = 0;

    // 描画処理：毎フレーム呼ばれる（画像の描画など）
    virtual void Draw() = 0;
};