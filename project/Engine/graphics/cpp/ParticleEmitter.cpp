#include "ParticleEmitter.h"
#include "GameConstants.h"
#include "ParticleManager.h"
#include <cstdlib>

ParticleEmitter::ParticleEmitter(const std::string& name, const Transform& transform)
    : name_(name)
    , transform_(transform)
{
}

void ParticleEmitter::Update()
{
    // 時刻を進める
    timeCount_ += GameConstants::kFrameDeltaTime;

    // 発生頻度より大きいなら発生
    if (timeCount_ >= frequency_) {
        // 発生処理
        Emit();

        timeCount_ -= frequency_;
    }
}

void ParticleEmitter::Emit()
{
    // ランダムな速度を生成
    Vector3 velocity = {
        (float)(rand() % 100 - 50) / 100.0f,
        (float)(rand() % 100 - 50) / 100.0f,
        (float)(rand() % 100 - 50) / 100.0f
    };

    ParticleManager::GetInstance()->Emit(name_, transform_.translate, velocity);
}