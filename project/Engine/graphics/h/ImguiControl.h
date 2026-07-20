/**
 * @file ImguiControl.h
 * @brief 開発・デバッグ用のImGuiコントロールパネル（UI）を表示するためのファイル
 */
#pragma once

/**
 * @brief ImGuiを使用したデバッグ用コントロールウィンドウを描画する
 * @note 毎フレームの更新処理（Update）内で、ImGuiManager::Begin() と ImGuiManager::End() の間に呼び出すこと。
 * この関数を呼ぶと、画面上にメッシュの切り替え、トランスフォーム（座標・回転・スケール）の操作、
 * マテリアルカラーやライティングモードの変更を行うためのUIウィンドウが表示されます。
 */
void ShowControls();