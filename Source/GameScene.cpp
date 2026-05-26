#include "GameScene.h"
#include "DxLib.h"
#include <cmath>
#include <string>
#include <fstream>  // 💡 追加：ファイル入力用
#include <sstream>  // 💡 追加：文字列解析用

GameScene::GameScene()
    : m_currentStage(nullptr)
    , m_currentStageIndex(0)
    , m_state(GameState::Playing)
    , m_clearTimer(0)
    , m_stateTransitionTimer(0.0f)
    , m_isDragging(false)
    , m_dragStartPos(VGet(0, 0, 0))
    , m_dragCurrentPos(VGet(0, 0, 0))
{
}

GameScene::~GameScene() {
    if (m_currentStage != nullptr) {
        delete m_currentStage;
    }
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
                currentStage.filters.push_back(Filter(
                    VGet(std::stof(x1), std::stof(y1), 0.0f),
                    VGet(std::stof(x2), std::stof(y2), 0.0f),
                    static_cast<LaserColor>(std::stoi(colorStr))
                ));
            }
        }

        file.close(); // ファイルを閉じる

    }

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
    // 全ステージクリア済みの場合は更新しない
    if (m_currentStageIndex >= (int)m_stages.size() || m_currentStage == nullptr) {
        return SceneName::None;
    }

    int mouseX, mouseY;
    GetMousePoint(&mouseX, &mouseY);
    m_dragCurrentPos = VGet((float)mouseX, (float)mouseY, 0.0f);

    static int prevMouseInput = 0;
    int mouseInput = GetMouseInput();
 
    if (m_state == GameState::Playing) {
        // 💡 1. 鏡の回転や削除などの重い処理はすべて Stage に丸投げ！
        m_currentStage->Update(m_dragCurrentPos, mouseInput, prevMouseInput);

        // 💡 2. 鏡の新規設置（ドラッグ）処理
        // 残り枚数に余裕があるときだけドラッグを許可する
        if (m_currentStage->GetRemainingMirrors() > 0) {
            if ((mouseInput & MOUSE_INPUT_LEFT) && !(prevMouseInput & MOUSE_INPUT_LEFT)) {
                m_isDragging = true;
                m_dragStartPos = m_dragCurrentPos;
            }
        }

        if (m_isDragging) {
            if (!(mouseInput & MOUSE_INPUT_LEFT)) {
                m_isDragging = false;

                // 💡 マウスを離したら、現在のステージに鏡を追加する！
                if (m_currentStage != nullptr) {
                    m_currentStage->AddMirror(m_dragStartPos, m_dragCurrentPos);
                    m_clearTimer = 0; // 鏡が増えた瞬間も開通タイマーをリセット
                }
            }
        }
    }
    else if (m_state == GameState::Clear) {
        // クリア演出タイマー
        m_stateTransitionTimer += 1.0f / 60.0f;
        if (m_stateTransitionTimer >= 2.0f) { // 2秒経ったら次へ
            m_currentStageIndex++;
            if (m_currentStageIndex < (int)m_stages.size()) {
                LoadStage(m_currentStageIndex);
            }
        }
    }

    prevMouseInput = mouseInput;
    return SceneName::None;
}

void GameScene::Draw() {
    if (m_currentStage == nullptr) return;

    // 💡 1. ステージオブジェクトとレーザーの描画（ここで最新のIsHit判定が走る）
    m_currentStage->Draw(m_isDragging, m_dragStartPos, m_dragCurrentPos);

    // 💡 2. 【超重要】描画が終わった直後の、最も新鮮なフラグでクリア判定を行う！
    // (※的オブジェクトに直接触る代わりに、的の状態を反映した結果を判定します。
    // 本来は m_currentStage 内の判定用ゲッターを呼ぶか、Stage::Drawの戻り値にするのが綺麗です)

    // --- GameScene.cpp の Draw() 内のクリア判定部分 ---
    if (m_state == GameState::Playing) {
        // 💡 Stageクラス経由で的がヒットしているか確認する
        if (m_currentStage != nullptr && m_currentStage->IsTargetHit()) {
            m_clearTimer++;
            if (m_clearTimer >= 30) {
                m_state = GameState::Clear;
                m_isDragging = false;
            }
        }
        else {
            m_clearTimer = 0;
        }
    }

    // UIの描画
    char buf[128];
    sprintf_s(buf, "STAGE %d / %d", m_currentStageIndex + 1, (int)m_stages.size());
    DrawString(10, 10, buf, GetColor(255, 255, 255));

    int remaining = m_currentStage->GetRemainingMirrors();
    unsigned int uiColor = (remaining > 0) ? GetColor(255, 255, 255) : GetColor(255, 100, 100);
    sprintf_s(buf, "手持ちの鏡：あと %d 枚 / %d 枚", remaining, m_currentStage->GetMaxMirrors());
    DrawString(10, 40, buf, uiColor);

    // ステージクリア時の画面エフェクト
    if (m_state == GameState::Clear) {
        DrawBox(0, 250, 800, 350, GetColor(0, 0, 0), TRUE);
        DrawString(340, 285, "STAGE CLEAR !!", GetColor(255, 255, 0));
    }
}