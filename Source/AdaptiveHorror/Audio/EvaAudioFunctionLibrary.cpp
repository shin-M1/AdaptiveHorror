#include "Audio/EvaAudioFunctionLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
USoundWaveProcedural* CreateEvaPrototypeTone(const UObject* Outer, const float Frequency, const float Duration,
    const float VolumeScale, const ESoundGroup SoundGroup)
{
    if (!Outer || Frequency <= 0.0f || Duration <= 0.0f || VolumeScale <= 0.0f)
    {
        return nullptr;
    }

    constexpr int32 SampleRate = 22050;
    constexpr int32 NumChannels = 1;
    const int32 SampleCount = FMath::Max(1, FMath::RoundToInt(SampleRate * Duration));

    TArray<int16> Samples;
    Samples.SetNumZeroed(SampleCount);

    const float Volume = FMath::Clamp(VolumeScale, 0.0f, 1.0f);
    for (int32 Index = 0; Index < SampleCount; ++Index)
    {
        const float T = static_cast<float>(Index) / static_cast<float>(SampleRate);
        const float NormalizedTime = static_cast<float>(Index) / static_cast<float>(SampleCount);
        const float Envelope = FMath::Clamp(1.0f - NormalizedTime, 0.0f, 1.0f);
        const float Harmonic = 0.35f * FMath::Sin(2.0f * PI * Frequency * 0.5f * T);
        const float Wave = (FMath::Sin(2.0f * PI * Frequency * T) + Harmonic) * Envelope * Volume;
        Samples[Index] = static_cast<int16>(FMath::Clamp(Wave, -1.0f, 1.0f) * 32767.0f);
    }

    USoundWaveProcedural* Sound = NewObject<USoundWaveProcedural>(GetTransientPackage());
    if (!Sound)
    {
        return nullptr;
    }

    Sound->SetSampleRate(SampleRate);
    Sound->NumChannels = NumChannels;
    Sound->Duration = Duration;
    Sound->SoundGroup = SoundGroup;
    Sound->bLooping = false;
    Sound->QueueAudio(reinterpret_cast<uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
    return Sound;
}
}

void UEvaAudioFunctionLibrary::PlayPrototypeTone2D(const UObject* WorldContextObject, const float Frequency,
    const float Duration, const float VolumeScale)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        return;
    }

    if (USoundWaveProcedural* Sound = CreateEvaPrototypeTone(WorldContextObject, Frequency, Duration,
        VolumeScale, SOUNDGROUP_UI))
    {
        UGameplayStatics::PlaySound2D(World, Sound, 1.0f);
    }
}

void UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(const UObject* WorldContextObject, const FVector& Location,
    const float Frequency, const float Duration, const float VolumeScale)
{
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        return;
    }

    if (USoundWaveProcedural* Sound = CreateEvaPrototypeTone(WorldContextObject, Frequency, Duration,
        VolumeScale, SOUNDGROUP_Effects))
    {
        UGameplayStatics::PlaySoundAtLocation(World, Sound, Location, 1.0f);
    }
}
