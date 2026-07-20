#include "MapChipField.h"
#include <fstream>
#include <sstream>
#include <random>
#include <cmath>

void MapChipField::LoadMapFromCSV(const std::string& filePath)
{
    // 既存のデータをクリア
    mapData_.clear();

    std::ifstream file(filePath);
    if (!file.is_open()) {
        return; // ファイルが開けなかったら戻る
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<MapChipType> row;
        std::stringstream ss(line);
        std::string cell;

        // カンマ区切りで1セルずつ読み込む
        while (std::getline(ss, cell, ',')) {
            if (!cell.empty()) {
                // 文字列を数値に変換
                int typeInt = std::stoi(cell);
                row.push_back(static_cast<MapChipType>(typeInt));
            }
        }
        mapData_.push_back(row);
    }
}

void MapChipField::Initialize(ModelCommon* modelCommon, const std::vector<Model*>& models)
{
    modelCommon_ = modelCommon;
    models_ = models;

    // 乱数の初期化
    std::random_device rd;
    gen_.seed(rd());
    distChance_ = std::uniform_int_distribution<int>(0, 99);
    distShape_  = std::uniform_int_distribution<int>(0, 17);

    coolTime_ = 0;
    currentShape_.clear();
    shapeStep_ = 0;
    nextGenerateX_ = 0.0f; // 画面左端から生成スタート
    mapChips_.clear(); // リストを空にする
    chipBaseY_.clear();
    totalColumnsGenerated_ = 0;
    waveTime_ = 0.0f;
    waveStrength_ = 0.0f;

    // 最初にゲーム開始時、画面の幅より少し多めに地形を一気に作っておく
    for (int i = 0; i < 40; ++i) {
        GenerateColumn(nextGenerateX_);
        nextGenerateX_ += kTileSize; // 1列作ったら1マス右へずれる
    }
}

void MapChipField::Update(float speedMultiplier, float waveStrength)
{
    float currentScrollSpeed = scrollSpeed_ * speedMultiplier;

    // ウェーブ強度をスムーズに補間
    waveStrength_ += (waveStrength - waveStrength_) * 0.05f;
    waveTime_ += 0.06f;

    // 調子に応じてスポーン確率を変動（Normal=8%, Good=約15%, Excellent=約25%）
    spawnChance_ = static_cast<int>(8.0f + waveStrength_ * 17.0f);

    // 全チップを左へ移動させ、ウェーブを適用し、画面外に出たら削除する
    size_t i = 0;
    for (auto it = mapChips_.begin(); it != mapChips_.end();) {
        float newX = (*it)->GetTransform().translate.x - currentScrollSpeed;

        // ベース位置（ウェーブなし）を transform に保持
        (*it)->SetPosition({ newX, chipBaseY_[i], 0.0f });

        // LocalMatrixにウェーブオフセットを乗せる
        if (waveStrength_ > 0.001f) {
            float waveOffset = std::sinf(newX * 0.8f + waveTime_) * waveStrength_ * 0.4f;
            Vector3 wavePos = { newX, chipBaseY_[i] + waveOffset, 0.0f };
            (*it)->SetLocalMatrix(MakeAffineMatrix(
                Vector3{ 1.0f, 1.0f, 1.0f },
                Vector3{ 0.0f, 0.0f, 0.0f },
                wavePos
            ));
        } else {
            (*it)->ClearLocalMatrix();
        }

        (*it)->Update();

        // 画面の左端に行ったらリストから完全に消す
        if (newX < -10.0f) {
            it = mapChips_.erase(it);
            chipBaseY_.erase(chipBaseY_.begin() + i);
        } else {
            ++it;
            ++i;
        }
    }

    // 次に生成する予定の場所も地形と一緒に左へスクロールさせる
    nextGenerateX_ -= currentScrollSpeed;

    // 右端の余白が少なくなったら新しい列を補充する
    while (nextGenerateX_ < 35.0f) {
        GenerateColumn(nextGenerateX_);
        nextGenerateX_ += kTileSize; // 1列補充したら生成予定地を右へズラす
    }
}

MapChipType MapChipField::GetMapType(uint32_t x, uint32_t y) const
{
    if (y >= GetRow() || x >= GetCol()) {
        return MapChipType::None;
    }
    return mapData_[y][x];
}

Vector3 MapChipField::GetWorldPos(uint32_t x, uint32_t y) const
{
    // 1ブロックを 2.0f サイズと仮定した計算例
    float blockSize = 2.0f;
    return {
        (float)x * blockSize,
        (float)(GetRow() - 1 - y) * blockSize,
        0.0f
    };
}

void MapChipField::Draw()
{
    for (auto& chip : mapChips_) {
        chip->Draw();
    }
}

void MapChipField::DrawShadow()
{
    for (auto& chip : mapChips_) {
        chip->DrawShadow();
    }
}


std::vector<MapChipField::ColumnProfile> MapChipField::PickRandomShape()
{
    // 地面系（offsetY=0-1, height<=2）と浮き系（offsetY>=2）を混在させる
    // すべて詰まらない形状のみ
    switch (distShape_(gen_)) {
    // ===== 地面系 =====
    case 0: // 低い壁 × 1列
        return { {0,1} };
    case 1: // 中壁 × 2列
        return { {0,2}, {0,2} };
    case 2: // 階段上り（低→高）
        return { {0,1}, {0,2} };
    case 3: // 階段下り（高→低）
        return { {0,2}, {0,1} };
    case 4: // ピラミッド（低・高・低）
        return { {0,1}, {0,2}, {0,1} };
    case 5: // 凸凹（battlements）× 4列
        return { {0,2}, {0,1}, {0,2}, {0,1} };
    case 6: // L字風（地面壁 + 上に浮き）
        return { {0,2}, {0,1}, {2,1} };
    case 7: // 小ブロック × 2
        return { {0,1}, {0,1} };

    // ===== 浮き系（offsetY>=2 → 底がy=3以上、下を走り抜けられる）=====
    case 8: // 低浮き × 3（横に長いプラットフォーム）
        return { {2,1}, {2,1}, {2,1} };
    case 9: // 高浮き × 2
        return { {3,2}, {3,2} };
    case 10: // 浮き上り階段（低→中→高）
        return { {2,1}, {3,1}, {4,1} };
    case 11: // 浮き下り階段（高→中→低）
        return { {4,1}, {3,1}, {2,1} };
    case 12: // 浮き山形（低・高・低）
        return { {2,1}, {3,1}, {2,1} };
    case 13: // 浮き千鳥（高低交互）× 4列
        return { {2,1}, {3,1}, {2,1}, {3,1} };
    case 14: // ワイド低浮き × 5列
        return { {2,1}, {2,1}, {2,1}, {2,1}, {2,1} };
    case 15: // 二段浮き × 3列（厚みのあるプラットフォーム）
        return { {2,2}, {2,2}, {2,2} };
    case 16: // 高低浮き交互（谷型）
        return { {3,1}, {2,1}, {3,1} };
    case 17: // 単独高浮き（1列だけ高いところに）
    default:
        return { {4,2} };
    }
}

void MapChipField::GenerateColumn(float x)
{
    totalColumnsGenerated_++;

    // 必ず一番下に床を生成する
    auto floorChip = std::make_unique<Object3d>();
    floorChip->Initialize(modelCommon_);
    floorChip->SetEnableLighting(false);
    floorChip->SetModel(models_[0]);
    floorChip->SetPosition({ x, 0.0f, 0.0f });
    floorChip->Update();
    mapChips_.push_back(std::move(floorChip));
    chipBaseY_.push_back(0.0f);

    // フラット地帯 or 絶不調のときは障害物を出さない
    if (totalColumnsGenerated_ < kFlatZoneColumns || terribleMode_) {
        coolTime_ = 0;
        currentShape_.clear();
        shapeStep_ = 0;
        return;
    }

    // クールタイム中なら障害物は置かない
    if (coolTime_ > 0) {
        coolTime_--;
        return;
    }

    // 形状の途中ならその列のプロフィールで生成
    auto spawnColumn = [&](const ColumnProfile& profile) {
        for (int h = 0; h < profile.height; ++h) {
            float posY = (1.0f + profile.offsetY + h) * kTileSize;
            auto obs = std::make_unique<Object3d>();
            obs->Initialize(modelCommon_);
            obs->SetEnableLighting(false);
            obs->SetModel(models_[1]);
            obs->SetPosition({ x, posY, 0.0f });
            obs->Update();
            mapChips_.push_back(std::move(obs));
            chipBaseY_.push_back(posY);
        }
    };

    if (shapeStep_ < (int)currentShape_.size()) {
        spawnColumn(currentShape_[shapeStep_]);
        shapeStep_++;
        if (shapeStep_ >= (int)currentShape_.size()) {
            coolTime_ = 4;
            currentShape_.clear();
            shapeStep_ = 0;
        }
        return;
    }

    // 新しい形状を開始するか判定（調子に応じて確率が変動）
    if (distChance_(gen_) < spawnChance_) {
        currentShape_ = PickRandomShape();
        shapeStep_ = 0;
        spawnColumn(currentShape_[shapeStep_]);
        shapeStep_++;
        if (shapeStep_ >= (int)currentShape_.size()) {
            coolTime_ = 4;
            currentShape_.clear();
            shapeStep_ = 0;
        }
    }
}