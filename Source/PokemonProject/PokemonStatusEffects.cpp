#include "PokemonStatusEffects.h"

namespace PokemonStatusEffectsInternal
{
	constexpr float FreezeThawChance = 20.0f; // % per turn
	constexpr float ParalysisFullyParalyzeChance = 25.0f; // % per turn
	constexpr float ParalysisAccuracyMultiplier = 0.75f;
	constexpr float BurnAttackMultiplier = 0.5f;
}

int32 UPokemonStatusEffects::RollInitialStatusCounter(const FString& StatusCondition)
{
	if (StatusCondition == TEXT("Sleep"))
	{
		return FMath::RandRange(1, 3);
	}
	return 0;
}

bool UPokemonStatusEffects::ProcessTurnStartStatus(const FString& StatusCondition, int32 StatusCounter,
	int32& NewStatusCounter, FString& NewStatusCondition, bool& bStatusJustCleared)
{
	using namespace PokemonStatusEffectsInternal;

	NewStatusCounter = StatusCounter;
	NewStatusCondition = StatusCondition;
	bStatusJustCleared = false;

	if (StatusCondition == TEXT("Sleep"))
	{
		NewStatusCounter = FMath::Max(StatusCounter - 1, 0);
		if (NewStatusCounter <= 0)
		{
			NewStatusCondition = TEXT("None");
			bStatusJustCleared = true;
			return true; // wakes up in time to act this turn
		}
		return false;
	}

	if (StatusCondition == TEXT("Freeze"))
	{
		if (FMath::FRandRange(0.0f, 100.0f) <= FreezeThawChance)
		{
			NewStatusCondition = TEXT("None");
			NewStatusCounter = 0;
			bStatusJustCleared = true;
			return true; // thaws in time to act this turn
		}
		return false;
	}

	if (StatusCondition == TEXT("Paralysis"))
	{
		return FMath::FRandRange(0.0f, 100.0f) > ParalysisFullyParalyzeChance;
	}

	// Burn, Poison, None: never prevents acting.
	return true;
}

float UPokemonStatusEffects::GetStatusAttackMultiplier(const FString& StatusCondition, const FString& MoveCategory)
{
	if (StatusCondition == TEXT("Burn") && MoveCategory == TEXT("Physical"))
	{
		return PokemonStatusEffectsInternal::BurnAttackMultiplier;
	}
	return 1.0f;
}

float UPokemonStatusEffects::GetStatusSpeedMultiplier(const FString& StatusCondition)
{
	if (StatusCondition == TEXT("Paralysis"))
	{
		return 0.25f;
	}
	return 1.0f;
}

float UPokemonStatusEffects::GetStatusAccuracyMultiplier(const FString& StatusCondition)
{
	if (StatusCondition == TEXT("Paralysis"))
	{
		return PokemonStatusEffectsInternal::ParalysisAccuracyMultiplier;
	}
	return 1.0f;
}

int32 UPokemonStatusEffects::GetStatusChipDamage(const FString& StatusCondition, int32 MaxHP)
{
	if (MaxHP <= 0)
	{
		return 0;
	}

	if (StatusCondition == TEXT("Poison"))
	{
		return FMath::Max(MaxHP / 8, 1);
	}
	if (StatusCondition == TEXT("Burn"))
	{
		return FMath::Max(MaxHP / 16, 1);
	}
	return 0;
}
