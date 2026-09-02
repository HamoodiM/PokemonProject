#include "PokemonHeldItems.h"

bool UPokemonHeldItemLibrary::ApplyFlatStatBonus(const FHeldItemEffect& Effect, const FString& StatName, int32 BaseStatValue, int32& NewStatValue)
{
	NewStatValue = BaseStatValue;

	if (Effect.EffectType != EHeldItemEffectType::FlatStatBonus || Effect.TargetStat != StatName)
	{
		return false;
	}

	NewStatValue = BaseStatValue + Effect.FlatStatBonus;
	return true;
}

float UPokemonHeldItemLibrary::GetDamageMultiplier(const FHeldItemEffect& Effect)
{
	if (Effect.EffectType != EHeldItemEffectType::DamageBoostWithRecoil)
	{
		return 1.0f;
	}
	return Effect.DamageMultiplier;
}

int32 UPokemonHeldItemLibrary::CalculateHeldItemRecoil(const FHeldItemEffect& Effect, int32 DamageDealt)
{
	if (Effect.EffectType != EHeldItemEffectType::DamageBoostWithRecoil || DamageDealt <= 0)
	{
		return 0;
	}
	return FMath::Max(FMath::CeilToInt(DamageDealt * Effect.RecoilFraction), 1);
}
