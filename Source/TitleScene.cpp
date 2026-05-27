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
    unsigned int colorRed = GetColor(255, 0, 0);
    unsigned int colorGreen = GetColor(0, 255, 0);
    unsigned int colorBlue = GetColor(0, 0, 255);
    unsigned int colorCyan = GetColor(0, 255, 255);
    unsigned int colorYellow = GetColor(255, 255, 0);
    unsigned int colorWhite = GetColor(255, 255, 255);

    // ----------------================================================-
    // 📺 1. アーケード定番の最上部スコア表示 (昭和フォント風)
    // ----------------================================================-
    SetFontSize(16);
    DrawString(50, 15, "1ST SCORE", colorWhite);
    DrawString(60, 35, "000000", colorWhite);

    DrawString(350, 15, "HI-SCORE", colorRed); // ハイスコアはなぜか赤文字が多い
    DrawString(360, 35, "099990", colorWhite);

    // ----------------================================================-
    // 🎨 2. ブラウン管の色ズレを再現した「ギラギラロゴ」
    // ----------------================================================-
    SetFontSize(54); // さらにデカく！
    std::string titleText = "LASER REFLECTION";
    int titleWidth = GetDrawStringWidth(titleText.c_str(), (int)titleText.size());
    int logoX = (Ut::SCREEN_WIDTH - titleWidth) / 2;
    int logoY = 160;

    // RGBの影をあえて大きくズラして重ねる（擬似・色にじみ効果）
    DrawString(logoX + 4, logoY + 4, titleText.c_str(), colorRed);   // 赤のズレ
    DrawString(logoX - 2, logoY - 2, titleText.c_str(), colorBlue);  // 青のズレ
    DrawString(logoX, logoY, titleText.c_str(), colorCyan);  // 本体のシアン

    // ----------------================================================-
    // 🌟 3. 点滅する「INSERT COIN」ガイド
    // ----------------================================================-
    SetFontSize(22);
    if ((m_flashTimer / 30) % 2 == 0) {
        std::string coinText = "INSERT COIN / PRESS SPACE KEY";
        int coinWidth = GetDrawStringWidth(coinText.c_str(), (int)coinText.size());
        DrawString((Ut::SCREEN_WIDTH - coinWidth) / 2, 360, coinText.c_str(), colorYellow);
    }
    else {
        // 点滅の裏側でうっすら文字のシルエットを残すのがレトロ筐体流
        std::string coinText = "INSERT COIN / PRESS SPACE KEY";
        int coinWidth = GetDrawStringWidth(coinText.c_str(), (int)coinText.size());
        DrawString((Ut::SCREEN_WIDTH - coinWidth) / 2, 360, coinText.c_str(), GetColor(50, 50, 0));
    }

    // ----------------================================================-
    // 🪙 4. 画面右下のクレジット表記
    // ----------------================================================-
    SetFontSize(16);
    DrawString(Ut::SCREEN_WIDTH - 160, Ut::SCREEN_HEIGHT - 35, "CREDIT  00", colorWhite);

    // 開発元（メーカーロゴっぽく原色で）
    DrawString(30, Ut::SCREEN_HEIGHT - 35, " DEVELOPER CORP.", GetColor(180, 180, 180));

    SetFontSize(originalFontSize);
}