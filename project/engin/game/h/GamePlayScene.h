/**
 * @file GamePlayScene.h
 * @brief ゲームプレイ本編のシーンロジックを管理するファイル
 */
#pragma once

 // --- 標準ライブラリ ---
#include <list>
#include <memory>
#include <string>
#include <vector>

// --- エンジンシステム・基盤 ---
#include "Audio.h"
#include "BaseScene.h"
#include "Camera.h"
#include "CollisionManager.h"
#include "DirectXCommon.h"
#include "GameObject.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "Model.h"
#include "ModelCommon.h"
#include "Object3dCommon.h"
#include "ShadowManager.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "SrvManager.h"

// --- ゲームロジック・オブジェクト ---
#include "Object3d.h"
#include "Bullet.h"
#include "EnemyManager.h"
#include "GameTime.h"
#include "MapChipField.h"
#include "Player.h"
#include "PlayerManager.h"
#include "Skydome.h"
#include "hoge.h"
#include "HitStarEmitter.h"
#include "Ring.h"
#include "Cylinder.h"
#include "SkinCommon.h"
#include "SkinnedModel.h"
#include "SkinnedObject3d.h"
#include "Skeleton.h"
#include "Animation.h"

/**
 * @brief ゲームプレイ本編のシーンクラス
 */
class GamePlayScene : public BaseScene{
public:
	// --- 基本関数 ---
	void Initialize(DirectXCommon* dxCommon,Input* input,Audio* audio) override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

	// --- パブリックメソッド ---
	void SetImGuiManager(ImGuiManager* imgui){ imguiManager_ = imgui; }

private:
	// --- 内部処理関数 ---
	void UpdateDebugUI();
	void DrawShadowPass();

	// --- データセーブ・ロード関数 ---
	void SaveCameraParams();
	void LoadCameraParams();
	void SaveEnemyParams();
	void LoadEnemyParams();
	void SaveModelPaths();
	void LoadModelPaths();
	void SaveUILayout();
	void LoadUILayout();

	// =================================================
	// メンバ変数
	// =================================================

	// --- 外部システムポインタ ---
	DirectXCommon* dxCommon_ = nullptr;
	Input* input_ = nullptr;
	Audio* audio_ = nullptr;
	ImGuiManager* imguiManager_ = nullptr;

	// --- 描画・共通基盤リソース ---
	std::unique_ptr<SpriteCommon> spriteCommon_;
	std::unique_ptr<ModelCommon> modelCommon_;
	std::unique_ptr<Object3dCommon> objectCommon_;
	std::unique_ptr<ShadowManager> shadowManager_;
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<CollisionManager> collisionManager_;

	// --- マネージャクラス ---
	std::unique_ptr<PlayerManager> playerManager_;
	std::unique_ptr<EnemyManager> enemyManager_;

	// --- 3Dモデル群 ---
	std::unique_ptr<Model> modelPlayer_;
	std::unique_ptr<Model> modelEnemy_;
	std::unique_ptr<Model> modelBullet_;
	std::unique_ptr<Model> modelBeam_;
	std::unique_ptr<Model> modelSkydome_;

	// --- 天球 ---
	std::unique_ptr<Skydome> skydome_;

	// --- スキンメッシュ（human） ---
	std::unique_ptr<SkinCommon>      skinCommon_;
	std::unique_ptr<SkinnedModel>    modelHuman_;
	std::unique_ptr<SkinnedObject3d> human_;

	// --- ゲームオブジェクト群 ---
	std::vector<std::unique_ptr<GameObject>> gameObjects_;
	std::list<std::unique_ptr<Bullet>> bullets_;
	std::unique_ptr<MapChipField> mapField_;

	// --- 進行・状態管理 ---
	GameTime gameTime_;

	// --- 楕円パーティクル（Ring 周回） ---
	float ellipseParticleTimer_ = 0.0f;
	static constexpr float kEllipseEmitInterval = 0.05f;
	float ringOrbitAngle_ = 0.0f; ///< パーティクル放出角度（リングを周回）

	// --- 星型ヒットエフェクト（常時発生） ---
	std::unique_ptr<HitStarEmitter> hitStarEmitter_;
	Vector3 hitStarPosition_ = { 12.0f, 2.0f, 0.0f };
	Vector4 hitStarColor_    = { 1.0f, 0.95f, 0.8f, 1.0f };
	float   hitStarFreq_     = 0.05f;

	// --- Ring ---
	std::unique_ptr<Ring> ring_;
	Vector3 ringPosition_    = { 14.5f, 0.0f, 0.0f };
	Vector3 ringRotation_    = {};
	Vector4 ringColor_       = { 1.0f, 1.0f, 1.0f, 1.0f };
	float   ringScale_       = 1.0f;
	float   ringInnerRadius_ = 1.5f;
	float   ringOuterRadius_ = 2.5f;

	// --- Cylinder ---
	std::unique_ptr<Cylinder> cylinder_;
	Vector3 cylinderPosition_     = { 17.0f, 0.0f, 0.0f };
	Vector3 cylinderRotation_     = {};
	Vector4 cylinderColor_        = { 1.0f, 1.0f, 1.0f, 1.0f };
	float   cylinderScale_        = 1.0f;
	float   cylinderTopRadius_    = 1.0f;
	float   cylinderBottomRadius_ = 1.0f;
	float   cylinderHeight_       = 3.0f;
	float   cylinderAlphaRef_     = 0.0f;

	// --- AnimatedCube ---
	Hoge*   animCube_          = nullptr;
	Vector3 animCubePosition_  = { 10.0f, 2.0f, 0.0f };
	float   animCubeSpeed_     = 1.0f;

	// --- Human パラメータ ---
	Vector3 humanPosition_      = { 5.0f, 0.0f, 0.0f };
	Vector3 humanRotation_      = { 0.0f, 0.0f, 0.0f };
	Vector3 humanScale_         = { 1.0f, 1.0f, 1.0f };
	float   humanAnimSpeed_     = 1.0f;
	bool    showSkeletonDebug_  = false;

	// --- Skydome パラメータ ---
	Vector4 skyColor_       = { 1.0f, 1.0f, 1.0f, 1.0f };
	float   skyRotOffsetY_  = 0.0f;

	// --- デバッグ・エディタ関連 ---
	bool debugScrollPaused_ = false;
	bool debugSpawnDisabled_ = false;
	bool debugEditMode_ = false;

	std::string playerObjPath_ = "Resources/player/player.obj";
	std::string playerTexPath_ = "Resources/player/player.png";
	std::string enemyObjPath_ = "Resources/boss/boss.obj";
	std::string enemyTexPath_ = "Resources/boss/boss.png";

	enum class SelectedType{ None,Player,Enemy,Camera,EnemySettings,UIElement,HitStar,Ring,Cylinder,Skydome,AnimatedCube,Human };
	SelectedType editorSelectedType_ = SelectedType::None;
	int editorSelectedIndex_ = -1;

	struct UIEntry{
		std::string name;
		std::unique_ptr<Sprite> sprite;
		std::string texPath;
	};
	std::vector<UIEntry> uiElements_;
};