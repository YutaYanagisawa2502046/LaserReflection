#include "TitleScene.h"
#include "DxLib.h"
#include "Utility.h" // 画面サイズ（Ut::SCREEN_WIDTH等）を使う場合
#include <string>

#include <cmath>

TitleScene::TitleScene()
    : m_flashTimer(0)
    , m_fontLarge(-1)
    , m_fontMedium(-1)
    , m_fontSmall(-1)
{
}

TitleScene::~TitleScene() {
    DeleteFontToHandle(m_fontLarge);
    DeleteFontToHandle(m_fontMedium);
    DeleteFontToHandle(m_fontSmall);
}

void TitleScene::Initialize() {
    m_flashTimer = 0;
    m_introFade = 0;

    // 💡 Windowsに標準搭載されているスタイリッシュなフォントを指定
    // 第4引数に DX_FONTTYPE_ANTIALIASING を渡すことで、輪郭が驚くほど滑らかになります！
    const char* fontName = "Segoe UI"; // または "Yu Gothic UI" もスマートでおすすめです

    m_fontLarge = CreateFontToHandle(fontName, 42, 1, DX_FONTTYPE_ANTIALIASING);
    m_fontMedium = CreateFontToHandle(fontName, 20, 1, DX_FONTTYPE_ANTIALIASING);
    m_fontSmall = CreateFontToHandle(fontName, 14, 1, DX_FONTTYPE_ANTIALIASING);
}

SceneName TitleScene::Update() {
    m_flashTimer++;

    // 💡 起動後、約0.8秒（45フレーム）かけて 1.0f に向かってじわっと増やす
    if (m_introFade < 1.0f) {
        m_introFade += 1.0f / 45.0f;
        if (m_introFade > 1.0f) m_introFade = 1.0f;
    }

    // フェードインが完全に終わるまではスペースキーを受け付けないようにするとより丁寧です
    if (m_introFade >= 1.0f) {
        if (CheckHitKey(KEY_INPUT_SPACE) || (GetMouseInput() & MOUSE_INPUT_LEFT)) {
            return SceneName::Game;
        }
    }
    return SceneName::None;
}

void TitleScene::Draw() {
    // モダンカラーの定義
    unsigned int colorMainText = GetColor(235, 240, 245); // オフホワイト
    unsigned int colorSubText = GetColor(140, 150, 160); // シックなグレー
    unsigned int colorAccent = GetColor(160, 210, 230); // ペールアクア（差し色）
    unsigned int colorLine = GetColor(60, 70, 80);  // 極細枠線用グレー

    // 💡 全体の描画にかけるアルファ値を m_introFade から計算
    int currentAlpha = static_cast<int>(255 * m_introFade);

    // 💡 描画全体の透明度をセット（これ以降の DrawString 等がすべてじわっと浮き出ます！）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, currentAlpha);

    // --- (ここから下の各種 DrawBox や DrawStringToHandle は以前のままでOK！) ---
    DrawBox(40, 40, Ut::SCREEN_WIDTH - 40, Ut::SCREEN_HEIGHT - 40, colorLine, FALSE);

    // ----------------================================================-
    // ✏️ タイトルロゴ（大フォント / 文字間を広く取ったモダン配置）
    // ----------------================================================-
    std::string titleText = "L A S E R   R E F L E C T I O N";

    // 横幅をハンドルから取得
    int titleWidth = GetDrawStringWidthToHandle(titleText.c_str(), (int)titleText.size(), m_fontLarge);

    // 💡 画面中央に正確に配置（縦軸は画面全体の 1/3 付近の Y=160 に設定）
    int titleX = (Ut::SCREEN_WIDTH - titleWidth) / 2;
    int titleY = 160;
    DrawStringToHandle(titleX, titleY, titleText.c_str(), colorMainText, m_fontLarge);

    // センターの飾りライン（ロゴの幅に合わせたサイズで中央引き）
    int lineHalfWidth = 100; // ラインの半分の長さ
    int centerX = Ut::SCREEN_WIDTH / 2;
    DrawLine(centerX - lineHalfWidth, titleY + 65, centerX + lineHalfWidth, titleY + 65, colorAccent);

    // サブタイトル
    std::string subText = "a minimalist light reflection puzzle";
    int subWidth = GetDrawStringWidthToHandle(subText.c_str(), (int)subText.size(), m_fontSmall);
    DrawStringToHandle((Ut::SCREEN_WIDTH - subWidth) / 2, titleY + 85, subText.c_str(), colorSubText, m_fontSmall);

    // ----------------================================================-
    // ⚪ スタート案内（中フォント ＆ サイン波による静かな呼吸明滅）
    // ----------------================================================-
    float alphaSin = std::sinf(m_flashTimer * 0.04f) * 0.5f + 0.5f;
    int textBright = 110 + static_cast<int>(110 * alphaSin); // 110〜220の間でじんわり変化
    unsigned int colorPress = GetColor(textBright, textBright, textBright + 15);

    std::string pressText = "Press Space to Begin";
    int pressWidth = GetDrawStringWidthToHandle(pressText.c_str(), (int)pressText.size(), m_fontMedium);

    // 💡 画面下部（Y=360付近）の中央に配置
    DrawStringToHandle((Ut::SCREEN_WIDTH - pressWidth) / 2, 360, pressText.c_str(), colorPress, m_fontMedium);

    // ----------------================================================-
    // 🏷️ フッタークレジット（小フォント / 枠線の内側にピッタリ沿わせる配置）
    // ----------------================================================-
    int footerY = Ut::SCREEN_HEIGHT - 70;

    // 左側：バージョン情報
    DrawStringToHandle(60, footerY, "v1.0.0 // stable build", colorLine, m_fontSmall);

    // 右側：開発者クレジット（右端の枠線から20px内側になるよう計算）
    std::string creditText = "designed by developer";
    int creditWidth = GetDrawStringWidthToHandle(creditText.c_str(), (int)creditText.size(), m_fontSmall);
    DrawStringToHandle(Ut::SCREEN_WIDTH - 40 - creditWidth - 20, footerY, creditText.c_str(), colorSubText, m_fontSmall);

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}