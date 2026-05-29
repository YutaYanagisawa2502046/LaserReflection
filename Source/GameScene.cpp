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
    , m_fontUiMain(-1) 
    , m_fontUiSub(-1)  
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
	m_stages.clear();   // 💡 既存のステージデータをクリアしてから新たに読み込む

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
				isReadingStage = true;  // 新しいステージの読み込み開始
            }
			else if (token == "STAGE_END") {// 1つのステージの読み込みが完了したら、データベースに保存して次のステージの準備
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

	// 💡 最初のステージを読み込む
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

	// ステージが切り替わるたびに、フェードイン状態から始めるようにする
    m_state = GameState::FadeIn;
    m_clearTimer = 0;
    m_stateTransitionTimer = 1.0f;
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
	// マウス座標を VECTOR 型に変換
    VECTOR mousePos = VGet(static_cast<float>(mouseX), static_cast<float>(mouseY), 0.0f);

    // 前フレームの入力（静的変数などで保持）
    static int prevMouseInput = 0;

    // ----------------================================================-
    // 🎮 状態 [A] : 通常プレイ中 (Playing)
    // ----------------================================================-
    if (m_state == GameState::Playing) {

        // 💡 ドラッグによる鏡の新規配置処理
		// マウスの左ボタンが押された瞬間を検出してドラッグ開始
        if ((mouseInput & MOUSE_INPUT_LEFT) && !(prevMouseInput & MOUSE_INPUT_LEFT)) {
            m_isDragging = true;
            m_dragStartPos = mousePos;
            m_dragCurrentPos = mousePos;
        }
		// ドラッグ中は現在のマウス位置を更新
        else if (m_isDragging && (mouseInput & MOUSE_INPUT_LEFT)) {
            m_dragCurrentPos = mousePos;
        }
		// マウスの左ボタンが離された瞬間を検出してドラッグ終了
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
			// 的から外れたら、クリアタイマーをリセットしてやり直し
            m_clearTimer = 0;
        }

        // デバッグ用の強制ステージスキップ（数字の「3」キーで次へ）
        if (CheckHitKey(KEY_INPUT_3)) {
			// 💡 強制的にクリア状態にして次のステージへ（デバッグ用）
            m_state = GameState::Clear;
            m_stateTransitionTimer = 0.0f;
        }
    }
    // ----------------================================================-
    // 🌟 状態 [B] : ステージクリア演出中 (Clear)
    // ----------------================================================-
    else if (m_state == GameState::Clear) {
		// 💡 クリア状態になったら、ドラッグ操作を無効化して、次のステージへの遷移タイマーをカウントアップ
        m_isDragging = false;
        m_stateTransitionTimer += 1.0f / 60.0f; // 💡 約0.5秒でフェードアウト（少し速くしました）

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
        m_stateTransitionTimer -= 1.0f / 60.0f; // 約0.5秒かけてじわっと戻る

        // 完全に不透明度が 0 以下になったら、通常プレイ状態へ移行
        if (m_stateTransitionTimer <= 0.0f) {
			// 💡 フェードインが完了したら、通常プレイ状態へ移行して、タイマーもリセット
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
		// クリアメッセージを3行表示
        std::string Clearstr[3] = {
            "ALL STAGE CLEAR !!!" ,
            "おめでとうございます！天才レーザーパズラー誕生です！" ,
            "【R】キーを押すと最初から遊べます"
        };

		static float clearAlpha = 0.0f; // クリアメッセージのアルファ値を管理する静的変数
		clearAlpha += 1.0f / 60.0f; // 約1.0秒かけて完全に表示されるようにアルファ値を増加させる
		if (clearAlpha > 1.0f) clearAlpha = 1.0f; // アルファ値を最大1.0fにクランプ

		// 💡 画面中央に大きく表示（フォントサイズは m_fontUiMain を使用）
		// 💡 文字列の幅と高さを取得して、中央に配置するための座標を計算
        int HeightCount = 0;
		const int LineDist = 10; // 行間を空けない場合は0、空ける場合は適宜数値を調整してください
        for (const auto& str : Clearstr) {
            int Width,Height;
            GetDrawStringSizeToHandle(&Width, &Height, NULL, str.c_str(), (int)str.size(), m_fontUiMain);
            
            int YCenter = (Height + LineDist) * 3;

			SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * clearAlpha)); // 💡 文字のアルファ値も全体のフェードイン比率に合わせる
            DrawStringToHandle(static_cast<int>((Ut::SCREEN_WIDTH - Width) * tmp),
                static_cast<int>((Ut::SCREEN_HEIGHT - YCenter) * tmp) + HeightCount,
                str.c_str(), GetColor(235, 240, 245), m_fontUiMain);
            HeightCount += Height + LineDist; // 行間を少し空ける
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); // ブレンドモードを元に戻す
        }

        if (CheckHitKey(KEY_INPUT_R))
        {
			Initialize();
            // 💡 Rキーを押すと最初からやり直せることを明示的にチェックしておく（Update内でもチェックしていますが、ここでもう一度）
        }
        return;
    }

    // =================================================================
    // 🎨 UI全体の「じわっと出現・じわっと消滅」比率計算
    // =================================================================
	// 💡 フェードイン・フェードアウトの状態に応じて、UI全体のアルファ比率を計算する
    float uiAlphaRatio = 1.0f;

    if (m_state == GameState::FadeIn) {
        // 💡 画面が明るくなる（タイマーが 1➔0 に減る）につれて、UIは 0➔1 へ「じわっと出現」
        uiAlphaRatio = 1.0f - m_stateTransitionTimer;
    }
    else if (m_state == GameState::Clear) {
        // 💡 画面が暗くなる（タイマーが 0➔1 に増える）につれて、UIは 1➔0 へ「じわっと消滅」
        // これによって、ステージクリア時に文字や枠線も一緒に闇へ溶けていきます！
        uiAlphaRatio = 1.0f - m_stateTransitionTimer;
    }

    // 安全のために 0.0f ～ 1.0f の範囲にクランプ
    if (uiAlphaRatio < 0.0f) uiAlphaRatio = 0.0f;
    if (uiAlphaRatio > 1.0f) uiAlphaRatio = 1.0f;

    // 💡 計算した比率をすべての色成分（RGB）に掛け算する
    unsigned int colorUiWhite = GetColor(static_cast<int>(235 * uiAlphaRatio),
        static_cast<int>(240 * uiAlphaRatio),
        static_cast<int>(245 * uiAlphaRatio));

    unsigned int colorUiGray = GetColor(static_cast<int>(140 * uiAlphaRatio),
        static_cast<int>(150 * uiAlphaRatio),
        static_cast<int>(160 * uiAlphaRatio));

    unsigned int colorUiAccent = GetColor(static_cast<int>(160 * uiAlphaRatio),
        static_cast<int>(210 * uiAlphaRatio),
        static_cast<int>(230 * uiAlphaRatio));

    unsigned int colorUiLine = GetColor(static_cast<int>(50 * uiAlphaRatio),
        static_cast<int>(58 * uiAlphaRatio),
        static_cast<int>(66 * uiAlphaRatio));

    unsigned int colorUiAlert = GetColor(240, 140, 140); // 警告色（鏡が0枚のとき）

    // 1. ステージ内の各種オブジェクト・レーザーの描画
    m_currentStage->Draw(m_isDragging, m_dragStartPos, m_dragCurrentPos);

    // =================================================================
    // 📊 2. 上部ヘッダーUI（現在の状況）
    // =================================================================
    // ステージ数表示（細線の上にスタイリッシュに配置）
	char stageBuf[32];  // 💡 ステージ番号を「STAGE 01 / 10」のようにフォーマットして表示
	sprintf_s(stageBuf, "STAGE  %02d  /  %02d", m_currentStageIndex + 1, (int)m_stages.size()); // 1から始まる表示にするために +1 しています
	DrawStringToHandle(30, 20, stageBuf, colorUiAccent, m_fontUiMain);  // 💡 画面の左上に配置（30pxの余白を空けて）

    // 残りの鏡枚数表示
	int remaining = m_currentStage->GetRemainingMirrors();  // 💡 残り枚数が0より多ければ通常色、0なら警告色で表示
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
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 50);
    DrawBox(30, footerY - 10, Ut::SCREEN_WIDTH, Ut::SCREEN_HEIGHT, GetColor(0, 0, 0), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 【左側カラム：操作説明】
    int leftX = 40;
    DrawStringToHandle(leftX, footerY, "[ CONTROLS ]", colorUiAccent, m_fontUiSub);
    DrawStringToHandle(leftX, footerY + 22, "Left Drag       : Place Mirror", colorUiGray, m_fontUiSub);
    DrawStringToHandle(leftX, footerY + 42, "Right Click     : Remove Mirror", colorUiGray, m_fontUiSub);
    DrawStringToHandle(leftX, footerY + 62, "Middle Click    : Fire Laser", colorUiGray, m_fontUiSub);
    DrawStringToHandle(leftX, footerY + 82, "Mouse Wheel     : Rotate Mirror ( 1° / +Shift: 0.1° )", colorUiWhite, m_fontUiSub);

    // 【右側カラム：パズルのルール】
    // 画面中央（Widthの半分）より少し右からスタートして綺麗にセパレート
    int rightX = Ut::SCREEN_WIDTH / 2 + 20;
    DrawStringToHandle(rightX, footerY, "[ SYSTEM RULES ]", colorUiAccent, m_fontUiSub);
    DrawStringToHandle(rightX, footerY + 22, "- Pass through filters to change laser color.", colorUiGray, m_fontUiSub);
    DrawStringToHandle(rightX, footerY + 42, "- Match the laser color with the target requirement.", colorUiGray, m_fontUiSub);
    DrawStringToHandle(rightX, footerY + 62, "- Hold the light on the target for 0.5s to clear.", colorUiGray, m_fontUiSub);

    float alpha = 255.0f * m_stateTransitionTimer;

	// 安全のために 0～255 の範囲にクランプして整数化
    int alphaInt = static_cast<int>(alpha);
    alphaInt = min(max(0, alphaInt), 255);

    // [B] ✨ 【目に優しい版】フェードアウト（Clear）とフェードイン（FadeIn）の画面マスク
    if (m_state == GameState::Clear || m_state == GameState::FadeIn) {
		// 💡 画面全体を覆うグレーの半透明マスク（アルファ値はタイマーに応じて変化）
        unsigned int colorFadeMask = GetColor(128, 128, 128);

        // アルファブレンドで画面全体を優しく包み込む
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alphaInt);
        DrawBox(0, 0, Ut::SCREEN_WIDTH, Ut::SCREEN_HEIGHT, colorFadeMask, TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
    
    // [A] クリア時の文字エフェクト
    if (m_state == GameState::Clear) {
        std::string str = "STAGE CLEAR";
        int SizeX = GetDrawStringWidthToHandle(str.c_str(), (int)str.size(), m_fontUiMain);

        // 💡 周りのUIとは独立して、「STAGE CLEAR」自体の輝度（0.0〜1.0）を計算
        // クリア演出の進行度（m_stateTransitionTimer：0➔1）に合わせて、
        // 最初の1/3（0.0〜0.33）のタイミングで一気にマックスまで明るくします！
        float textAlphaRatio = m_stateTransitionTimer * 3.0f;
        if (textAlphaRatio > 1.0f) textAlphaRatio = 1.0f; // マックスで固定

        // 💡 画面が完全に暗転する直前（0.8〜1.0）だけ、闇に溶けるように少しだけ減衰させる
        if (m_stateTransitionTimer > 0.8f) {
            textAlphaRatio = (1.0f - m_stateTransitionTimer) / 0.2f;
        }

        // 💡 周りに流されず、最後までパキッと発光する美しいシアン（ペールアクア）
        unsigned int colorClearText = GetColor(static_cast<int>(160 * textAlphaRatio),
            static_cast<int>(210 * textAlphaRatio),
            static_cast<int>(230 * textAlphaRatio));

        // 文字の背後の黒い帯の不透明度も、文字の浮き上がりに同期させる
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(180 * textAlphaRatio));
        DrawBox(0, Ut::SCREEN_HEIGHT / 2 - 40, Ut::SCREEN_WIDTH, Ut::SCREEN_HEIGHT / 2 + 40, GetColor(10, 15, 20), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        // 確定した色で「STAGE CLEAR」を描画
        DrawStringToHandle(Ut::SCREEN_WIDTH / 2 - SizeX / 2,
            Ut::SCREEN_HEIGHT / 2 - 15,
            str.c_str(), colorClearText, m_fontUiMain);
    }
}