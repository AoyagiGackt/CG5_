/**
 * @file GamePlayScene.h
 * @brief ゲームプレイ本編のシーンロジックを管理するファイル
 */
#pragma once

 // --- 標準ライブラリ ---
#include <deque>
#include <memory>
#include <string>
#include <vector>

// --- エンジンシステム・基盤 ---
#include "Audio.h"
#include "BaseScene.h"
#include "Camera.h"
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
#include "GameTime.h"
#include "MapChipField.h"
#include "Skydome.h"
#include "hoge.h"
#include "Ring.h"
#include "Cylinder.h"
#include "SkinCommon.h"
#include "SkinnedModel.h"
#include "SkinnedObject3d.h"
#include "Skeleton.h"
#include "Animation.h"
#include "ImageFilter.h"
#include "RenderTexture.h"

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

	// --- 3Dモデル群 ---
	std::unique_ptr<Model> modelSkydome_;

	// --- 天球 ---
	std::unique_ptr<Skydome> skydome_;

	// --- スキンメッシュ（human） ---
	std::unique_ptr<SkinCommon>      skinCommon_;
	std::unique_ptr<SkinnedModel>    modelHuman_;
	std::unique_ptr<SkinnedObject3d> human_;

	// --- ゲームオブジェクト群 ---
	std::vector<std::unique_ptr<GameObject>> gameObjects_;
	std::unique_ptr<MapChipField> mapField_;

	// --- 進行・状態管理 ---
	GameTime gameTime_;

	// --- 白パーティクル（1024個静的配置） ---
	Vector3  whiteParticlePos_   = { 14.5f, 3.0f, 0.0f };
	Vector4  whiteParticleColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	float    whiteParticleScale_ = 0.5f;
	int      whiteParticleCount_ = 1024;

	// --- 楕円パーティクル（Ring 周回） ---
	float ellipseParticleTimer_ = 0.0f;
	static constexpr float kEllipseEmitInterval = 0.05f;
	float ringOrbitAngle_ = 0.0f; ///< パーティクル放出角度（リングを周回）

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

	// --- オフスクリーンレンダリング ---
	std::unique_ptr<RenderTexture> renderTexture_;
	std::unique_ptr<Sprite>        renderTextureSprite_;

	// --- Camera Box Filter Smoothing ---
	Vector3 cameraTargetPos_ = { 14.5f, 6.0f, -30.0f };
	Vector3 cameraTargetRot_ = { 0.0f, 0.0f, 0.0f };
	std::deque<Vector3> cameraPosHistory_;
	std::deque<Vector3> cameraRotHistory_;
	int cameraSmoothFrames_ = 1; // 1=即時, N=過去Nフレーム平均

	// --- デバッグ・エディタ関連 ---
	bool debugEditMode_ = false;

	enum class SelectedType{ None,Camera,UIElement,Ring,Cylinder,Skydome,AnimatedCube,Human,WhiteParticles };
	SelectedType editorSelectedType_ = SelectedType::None;
	int editorSelectedIndex_ = -1;

	struct UIEntry{
		std::string name;
		std::unique_ptr<Sprite> sprite;
		std::string texPath;
	};
	std::vector<UIEntry> uiElements_;
};