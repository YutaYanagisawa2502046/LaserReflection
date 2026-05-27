#include "TitleScene.h"
#include "DxLib.h"
#include "Utility.h" // 画面サイズ（Ut::SCREEN_WIDTH等）を使う場合
#include <string>

TitleScene::TitleScene() : m_flashTimer(0) {}

void TitleScene::Initialize() {
    m_flashTimer = 0;
}

SceneName TitleScene::Update() {
    m_flashTimer++;

    // 💡 スペースキー（または左クリック）が押されたら、ゲームメイン画面（GameScene）へ遷移！
    if (CheckHitKey(KEY_INPUT_SPACE) || (GetMouseInput() & MOUSE_INPUT_LEFT)) {
        return SceneName::Game; // 💡 お使いの環境の「ゲームシーンを表す列挙型」を返してください
    }

    return SceneName::None; // 何もなければタイトルを維持
}

void TitleScene::Draw() {
    int originalFontSize = GetFontSize();

    // ----------------================================================-
    // 🎨 1. 今時風の洗練された「サスティナブル・ミニマム」な配色
    // ----------------================================================-
    // 背景が黒ではなく、少し青み・赤みのある高級なダークグレーを想定
    unsigned int colorMainText = GetColor(235, 240, 245); // オフホワイト（目に優しい白）
    unsigned int colorSubText = GetColor(140, 150, 160); // シックなセメントグレー
    unsigned int colorAccent = GetColor(160, 210, 230); // 淡いペールアクア（差し色）
    unsigned int colorLine = GetColor(60, 70, 80);  // 極細の枠線用グレー

    // ----------------================================================-
    // 🖼️ 2. 幾何学的なモダン・背景アクセント（薄く細い線でパズル感を演出）
    // ----------------================================================-
    // 画面中央に、デザインの一部として非常に細い飾り枠線を描く
    DrawBox(40, 40, Ut::SCREEN_WIDTH - 40, Ut::SCREEN_HEIGHT - 40, colorLine, FALSE);

    // ----------------================================================-
    // ✏️ 3. スタイリッシュなタイトルロゴ（小さめ、細め、広いレタースペース風）
    // ----------------================================================-
    // あえてフォントサイズを「36」程度に抑え、余白の美しさを強調します
    SetFontSize(36);
    std::string titleText = "L A S E R   R E F L E C T I O N"; // 💡文字間にスペースを空けるのが今時！
    int titleWidth = GetDrawStringWidth(titleText.c_str(), (int)titleText.size());
    DrawString((Ut::SCREEN_WIDTH - titleWidth) / 2, 180, titleText.c_str(), colorMainText);

    // 飾り用のアンダーライン（極細の一本線）
    DrawLine((Ut::SCREEN_WIDTH - 200) / 2, 240, (Ut::SCREEN_WIDTH + 200) / 2, 240, colorAccent);

    // コンセプト文（サブタイトル）
    SetFontSize(14);
    std::string subText = "a minimalist light reflection puzzle";
    int subWidth = GetDrawStringWidth(subText.c_str(), (int)subText.size());
    DrawString((Ut::SCREEN_WIDTH - subWidth) / 2, 260, subText.c_str(), colorSubText);

    // ----------------================================================-
    // ⚪ 4. 静かに佇むスタート案内（点滅はさせず、静かに呼吸するような明度変化）
    // ----------------================================================-
    SetFontSize(16);
    // サイン波を使って、文字の明るさを滑らかに変化させる（1/60秒ずつじんわり明滅）
    float alphaSin = sinf(m_flashTimer * 0.04f) * 0.5f + 0.5f;
    int textBright = 100 + static_cast<int>(100 * alphaSin); // 100〜200の間で滑らかに変化
    unsigned int colorPress = GetColor(textBright, textBright, textBright + 20);

    std::string pressText = "Press Space to Begin";
    int pressWidth = GetDrawStringWidth(pressText.c_str(), (int)pressText.size());
    DrawString((Ut::SCREEN_WIDTH - pressWidth) / 2, 380, pressText.c_str(), colorPress);

    // ----------------================================================-
    // 🏷️ 5. フッター（極小サイズでミニマルに配置）
    // ------------------------------------------------================-
    SetFontSize(12);
    DrawString(60, Ut::SCREEN_HEIGHT - 70, "v1.0.0 // stable build", colorLine);
    DrawString(Ut::SCREEN_WIDTH - 220, Ut::SCREEN_HEIGHT - 70, "designed by developer", colorSubText);

    SetFontSize(originalFontSize);
}