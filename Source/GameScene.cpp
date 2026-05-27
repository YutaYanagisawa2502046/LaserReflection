#include "GameScene.h"
#include "DxLib.h"
#include <cmath>
#include <string>
#include <fstream>  // 💡 追加：ファイル入力用
#include <sstream>  // 💡 追加：文字列解析用
#include "Utility.h"

GameScene::GameScene()
    : m_currentStage(nullptr)
    , m_currentStageIndex(0)
    , m_state(GameState::Playing)
    , m_clearTimer(0)
    , m_stateTransitionTimer(0.0f)
    , m_isDragging(false)
    , m_dragStartPos(VGet(0, 0, 0))
    , m_dragCurrentPos(VGet(0, 0, 0))
    , m_fontUiMain(-1) // 💡 初期化
    , m_fontUiSub(-1)  // 💡 初期化
{
}

GameScene::~GameScene() {
    if (m_currentStage != nullptr) {
        delete m_currentStage;
    }
    // 💡 生成したフォントハンドルを安全に解放
    DeleteFontToHandle(m_fontUiMain);
    DeleteFontToHandle(m_fontUiSub);
}

void GameScene::Initialize() {
    m_stages.clear();

    {
        // 💡 1. 実行ファイルと同じフォルダにある「StageData.txt」を開く
        std::ifstream file("Stage/StageData.txt");
        if (!file.is_open()) {
            // ファイルが見つからない場合はポップアップで警告して終了
            MessageBox(NULL, "StageData.txt が見つかりません！\nexeと同じフォルダに作成してください。", "Error", MB_OK);
            return;
        }

        std::string line;
        StageData currentStage;
        bool isReadingStage = false;

        // 💡 2. メモ帳を1行ずつループで読み込む
        while (std::getline(file, line)) {
            // 空行や、'#' から始まるコメント行は処理を飛ばす
            if (line.empty() || line[0] == '#') continue;

            // カンマ「,」で文字を細かく分解する準備
            std::stringstream ss(line);
            std::string token;
            std::getline(ss, token, ','); // 行の先頭の単語（LASER や TARGET など）を取得

            if (token == "STAGE_START") {
                currentStage = StageData(); // 構造体をきれいにリセット
                isReadingStage = true;
            }
            else if (token == "STAGE_END") {
                if (isReadingStage) {
                    m_stages.push_back(currentStage); // 解析が終わったステージデータをデータベースに保存！
                    isReadingStage = false;
                }
            }
            else if (token == "LASER") {
                std::string x, y, dx, dy, colorStr;
                std::getline(ss, x, ','); std::getline(ss, y, ',');
                std::getline(ss, dx, ','); std::getline(ss, dy, ',');
                std::getline(ss, colorStr, ','); // 💡 5つ目の要素（色）を取得

                currentStage.laserPos = VGet(std::stof(x), std::stof(y), 0.0f);
                currentStage.laserDir = VGet(std::stof(dx), std::stof(dy), 0.0f);

                // 💡 色が指定されていれば読み込み、なければデフォルトで赤(0)にする安全設計
                if (!colorStr.empty()) {
                    currentStage.initialColor = static_cast<LaserColor>(std::stoi(colorStr));
                }
                else {
                    currentStage.initialColor = LaserColor::Red;
                }
            }
            else if (token == "TARGET") {
                std::string x, y, radius, colorStr;
                std::getline(ss, x, ','); std::getline(ss, y, ',');
                std::getline(ss, radius, ','); std::getline(ss, colorStr, ',');
                currentStage.targetPos = VGet(std::stof(x), std::stof(y), 0.0f);
                currentStage.targetRadius = std::stof(radius);

                // テキスト側の「0, 1, 2」を LaserColor の列挙型に安全に変換
                currentStage.requiredColor = static_cast<LaserColor>(std::stoi(colorStr));
            }
            else if (token == "MAX_MIRRORS") {
                std::string maxM;
                std::getline(ss, maxM, ',');
                currentStage.maxMirrors = std::stoi(maxM);
            }
            else if (token == "OBSTACLE") {
                std::string x1, y1, x2, y2;
                std::getline(ss, x1, ','); std::getline(ss, y1, ',');
                std::getline(ss, x2, ','); std::getline(ss, y2, ',');
                currentStage.obstacles.push_back(Obstacle(
                    VGet(std::stof(x1), std::stof(y1), 0.0f),
                    VGet(std::stof(x2), std::stof(y2), 0.0f)
                ));
            }
            else if (token == "FILTER") {
                std::string x1, y1, x2, y2, colorStr;
                std::getline(ss, x1, ','); std::getline(ss, y1, ',');
                std::getline(ss, x2, ','); std::getline(ss, y2, ',');
                std::getline(ss, colorStr, ',');

                LaserColor filterColor = LaserColor::Red; // デフォルト安全値
                if (!colorStr.empty()) {
                    filterColor = static_cast<LaserColor>(std::stoi(colorStr));
                }
                currentStage.filters.push_back(Filter(
                    VGet(std::stof(x1), std::stof(y1), 0.0f),
                    VGet(std::stof(x2), std::stof(y2), 0.0f),
                    static_cast<LaserColor>(filterColor)
                ));
            }
        }

        file.close(); // ファイルを閉じる

    }

    // 💡 タイトル画面と完全に同じフォント・品質でUI用フォントを生成
    const char* fontName = "Segoe UI"; // または "Yu Gothic UI"
    m_fontUiMain = CreateFontToHandle(fontName, 26, 1, DX_FONTTYPE_ANTIALIASING);
    m_fontUiSub = CreateFontToHandle(fontName, 14, 1, DX_FONTTYPE_ANTIALIASING);

    m_currentStageIndex = 0;
    LoadStage(m_currentStageIndex);
}

void GameScene::LoadStage(int index) {
    // 古いステージ実体を削除
    if (m_currentStage != nullptr) {
        delete m_currentStage;
        m_currentStage = nullptr;
    }

    // データベースから新しいステージをインスタンス化
    if (index < (int)m_stages.size()) {
        m_currentStage = new Stage(m_stages[index]);
    }

    m_state = GameState::Playing;
    m_clearTimer = 0;
    m_stateTransitionTimer = 0.0f;
    m_isDragging = false;
}

SceneName GameScene::Update() {
    // 💡 1. 全ステージクリア時は、Rキーで最初からリトライできるようにする
    if (m_currentStageIndex >= (int)m_stages.size() || m_currentStage == nullptr) {
        if (CheckHitKey(KEY_INPUT_R)) {
            Initialize(); // 最初からやり直し
        }
        return SceneName::None;
    }

    // マウス入力情報の取得（Ut:: 等、お使いの環境に合わせて適宜調整してください）
    int mouseInput = GetMouseInput();
    int mouseX, mouseY;
    GetMousePoint(&mouseX, &mouseY);
    VECTOR mousePos = VGet(static_cast<float>(mouseX), static_cast<float>(mouseY), 0.0f);

    // 前フレームの入力（静的変数などで保持）
    static int prevMouseInput = 0;

    // ----------------================================================-
    // 🎮 状態 [A] : 通常プレイ中 (Playing)
    // ----------------================================================-
    if (m_state == GameState::Playing) {

        // 💡 ドラッグによる鏡の新規配置処理
        if ((mouseInput & MOUSE_INPUT_LEFT) && !(prevMouseInput & MOUSE_INPUT_LEFT)) {
            m_isDragging = true;
            m_dragStartPos = mousePos;
            m_dragCurrentPos = mousePos;
        }
        else if (m_isDragging && (mouseInput & MOUSE_INPUT_LEFT)) {
            m_dragCurrentPos = mousePos;
        }
        else if (m_isDragging && !(mouseInput & MOUSE_INPUT_LEFT)) {
            m_isDragging = false;
            // ドラッグ終了時にステージに鏡を追加
            m_currentStage->AddMirror(m_dragStartPos, m_dragCurrentPos);
        }

        // 💡 ステージ全体の更新（鏡の選択・回転・右クリック削除など）
        m_currentStage->Update(mousePos, mouseInput, prevMouseInput);

        // 💡 【超重要】的へのヒット判定とクリアタイマーのカウント
        if (m_currentStage->IsTargetHit()) {
            m_clearTimer++;
            if (m_clearTimer >= 30) {
                m_state = GameState::Clear;
                m_stateTransitionTimer = 0.0f; // 0.0f（透明）からスタート
            }
        }
        else {
            m_clearTimer = 0;
        }

        // デバッグ用の強制ステージスキップ（数字の「3」キーで次へ）
        if (CheckHitKey(KEY_INPUT_3)) {
            m_state = GameState::Clear;
            m_stateTransitionTimer = 0.0f;
        }
    }
    // ----------------================================================-
    // 🌟 状態 [B] : ステージクリア演出中 (Clear)
    // ----------------================================================-
    else if (m_state == GameState::Clear) {
        m_isDragging = false;
        m_stateTransitionTimer += 1.0f / 30.0f; // 💡 約0.5秒でフェードアウト（少し速くしました）

        if (m_stateTransitionTimer >= 1.0f) {
            m_currentStageIndex++; // 次のステージへ

            if (m_currentStageIndex < (int)m_stages.size()) {
                LoadStage(m_currentStageIndex); // ステージ読み込み

                // 💡 【重要】ここを Playing ではなく FadeIn にする！
                m_state = GameState::FadeIn;
                m_stateTransitionTimer = 1.0f; // 1.0f（真っ白）からスタート
            }
            else {
                if (m_currentStage != nullptr) {
                    delete m_currentStage;
                    m_currentStage = nullptr;
                }
            }
        }
    }
    // ----------------================================================-
    // ✨ 状態 [C] : 新ステージ開始➔フェードイン中 (FadeIn)
    // ----------------================================================-
    else if (m_state == GameState::FadeIn) {
        // タイマーを 1.0f から 0.0f に向かって減算していく
        m_stateTransitionTimer -= 1.0f / 30.0f; // 約0.5秒かけてじわっと戻る

        // 完全に不透明度が 0 以下になったら、通常プレイ状態へ移行
        if (m_stateTransitionTimer <= 0.0f) {
            m_stateTransitionTimer = 0.0f;
            m_state = GameState::Playing;
        }
    }

    prevMouseInput = mouseInput; // マウス状態の保存
    return SceneName::None;
}

void GameScene::Draw() {
    // ---- エンディング画面（全ステージクリア時） ----
    if (m_currentStageIndex >= (int)m_stages.size() || m_currentStage == nullptr) {
        float tmp = 1.0f / 2.0f;
        std::string Clearstr[3] = {
            "ALL STAGE CLEAR !!!" ,
            "おめでとうございます！天才レーザーパズラー誕生です！" ,
            "【R】キーを押すと最初から遊べます"
        };

        int aaa = 0;
        for (const auto& str : Clearstr) {
            int Width = GetDrawStringWidthToHandle(str.c_str(), (int)str.size(), m_fontUiMain);
            DrawStringToHandle(static_cast<int>((Ut::SCREEN_WIDTH - Width) * tmp),
                static_cast<int>(Ut::SCREEN_HEIGHT * tmp + (40 * aaa)),
                str.c_str(), GetColor(235, 240, 245), m_fontUiMain);
            aaa++;
        }
        return;
    }

    // 🎨 モダンカラーの定義（タイトル画面の世界観を踏襲）
    unsigned int colorUiWhite = GetColor(235, 240, 245); // メイン文字
    unsigned int colorUiGray = GetColor(140, 150, 160); // 補助文字
    unsigned int colorUiAccent = GetColor(160, 210, 230); // 差し色（シアン系）
    unsigned int colorUiLine = GetColor(50, 58, 66);  // 区切り線用の極細グレー
    unsigned int colorUiAlert = GetColor(240, 140, 140); // 警告色（鏡が0枚のとき）

    // 1. ステージ内の各種オブジェクト・レーザーの描画
    m_currentStage->Draw(m_isDragging, m_dragStartPos, m_dragCurrentPos);

    // =================================================================
    // 📊 2. 上部ヘッダーUI（現在の状況）
    // =================================================================
    // ステージ数表示（細線の上にスタイリッシュに配置）
    char stageBuf[32];
    sprintf_s(stageBuf, "STAGE  %02d  /  %02d", m_currentStageIndex + 1, (int)m_stages.size());
    DrawStringToHandle(30, 20, stageBuf, colorUiAccent, m_fontUiMain);

    // 残りの鏡枚数表示
    int remaining = m_currentStage->GetRemainingMirrors();
    unsigned int mirrorTextColor = (remaining > 0) ? colorUiWhite : colorUiAlert;
    char buf[64];
    sprintf_s(buf, "MIRRORS  :  %d  /  %d", remaining, m_currentStage->GetMaxMirrors());
    // 画面の右上に綺麗に整列させる（右端から200pxの位置）
    DrawStringToHandle(Ut::SCREEN_WIDTH - 200, 26, buf, mirrorTextColor, m_fontUiSub);

    // 上部とゲーム画面を隔てる美しいセパレート細線
    DrawLine(30, 60, Ut::SCREEN_WIDTH - 30, 60, colorUiLine);

    // =================================================================
    // ⌨️ 3. 下部フッターUI（常時操作説明 ＆ グリッド配置）
    // =================================================================
    int footerY = Ut::SCREEN_HEIGHT - 110;

    // 下部を隔てるセパレート細線
    DrawLine(30, footerY - 10, Ut::SCREEN_WIDTH - 30, footerY - 10, colorUiLine);

    // 【左側カラム：操作説明】
    int leftX = 40;
    DrawStringToHandle(leftX, footerY, "[ CONTROLS ]", colorUiAccent, m_fontUiSub);
    DrawStringToHandle(leftX, footerY + 22, "Left Drag       : Place Mirror", colorUiGray, m_fontUiSub);
    DrawStringToHandle(leftX, footerY + 42, "Right Click     : Remove Mirror", colorUiGray, m_fontUiSub);
    DrawStringToHandle(leftX, footerY + 62, "Mouse Wheel     : Rotate Mirror ( 1° / +Shift: 0.1° )", colorUiWhite, m_fontUiSub);

    // 【右側カラム：パズルのルール】
    // 画面中央（Widthの半分）より少し右からスタートして綺麗にセパレート
    int rightX = Ut::SCREEN_WIDTH / 2 + 20;
    DrawStringToHandle(rightX, footerY, "[ SYSTEM RULES ]", colorUiAccent, m_fontUiSub);
    DrawStringToHandle(rightX, footerY + 22, "- Pass through filters to change laser color.", colorUiGray, m_fontUiSub);
    DrawStringToHandle(rightX, footerY + 42, "- Match the laser color with the target requirement.", colorUiGray, m_fontUiSub);
    DrawStringToHandle(rightX, footerY + 62, "- Hold the light on the target for 0.5s to clear.", colorUiGray, m_fontUiSub);


    // [A] クリア時の文字エフェクト（Clearのときだけ帯を出す）
    if (m_state == GameState::Clear) {
        std::string str = "STAGE CLEAR";
        int SizeX = GetDrawStringWidthToHandle(str.c_str(), (int)str.size(), m_fontUiMain);

        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawBox(0, Ut::SCREEN_HEIGHT / 2 - 40, Ut::SCREEN_WIDTH, Ut::SCREEN_HEIGHT / 2 + 40, GetColor(15, 20, 25), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawStringToHandle(Ut::SCREEN_WIDTH / 2 - SizeX / 2, Ut::SCREEN_HEIGHT / 2 - 15, str.c_str(), colorUiAccent, m_fontUiMain);
    }

    // [B] ✨ フェードアウト（Clear）とフェードイン（FadeIn）の画面マスク
    if (m_state == GameState::Clear || m_state == GameState::FadeIn) {
        // m_stateTransitionTimer は Clear の時は 0➔1、FadeIn の時は 1➔0 に変化します
        float alpha = 255.0f * m_stateTransitionTimer;

        // 安全のために 0 〜 255 の範囲にクランプ
        int alphaInt = static_cast<int>(alpha);
        if (alphaInt < 0) alphaInt = 0;
        if (alphaInt > 255) alphaInt = 255;

        // 💡 アルファブレンドで画面全体を覆う（タイトル等と合わせたオフホワイト）
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alphaInt);
        DrawBox(0, 0, Ut::SCREEN_WIDTH, Ut::SCREEN_HEIGHT, GetColor(235, 240, 245), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
}