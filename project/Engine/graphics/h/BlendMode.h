#pragma once

enum BlendMode {
    BlendMode_None, // ブレンドなし（不透明）
    BlendMode_Alpha, // 通常の半透明（アルファブレンド）
    BlendMode_Add, // 加算合成（エフェクト等）
    BlendMode_Subtract, // 減算合成
    BlendMode_Multiply, // 乗算合成
    BlendMode_Count // 総数
};