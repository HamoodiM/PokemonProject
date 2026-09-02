#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PokemonHeldItems.generated.h"

UENUM(BlueprintType)
enum class EHeldItemEffectType : uint8
{
	None,
	/** Assault Vest-style: flat bonus to one named stat. */
	FlatStatBonus,
	/** Life Orb-style: multiplies damage dealt, at the cost of recoil to the holder. */
	DamageBoostWithRecoil
};

/**
 * Data-driven held-item effect, same broken-out-primitives convention as
 * UPokemonBattleLibrary/UPokemonMoveEffects. One item = one FHeldItemEffect (or None for items
 * with no battle effect, e.g. evolution stones/status cures which have their own systems already).
 */
USTRUCT(BlueprintType)
struct FHeldItemEffect
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|HeldItem")
	EHeldItemEffectType EffectType = EHeldItemEffectType::None;

	/** FlatStatBonus only: "Attack", "Defence", "SpAttack", "SpDefence", "Speed". */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|HeldItem")
	FString TargetStat;

	/** FlatStatBonus only. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|HeldItem")
	int32 FlatStatBonus = 0;

	/** DamageBoostWithRecoil only, e.g. 1.3 for Life Orb's +30% damage. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|HeldItem")
	float DamageMultiplier = 1.0f;

	/** DamageBoostWithRecoil only: fraction of damage dealt the holder takes back, e.g. 0.1 for Life Orb. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|HeldItem")
	float RecoilFraction = 0.0f;
};

/** Pure C++ resolvers for FHeldItemEffect. Caller applies the results to the actual battle stat calc. */
UCLASS()
class POKEMONPROJECT_API UPokemonHeldItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * No-op (returns false, NewStatValue=BaseStatValue) unless EffectType is FlatStatBonus and
	 * StatName matches Effect.TargetStat.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|HeldItem")
	static bool ApplyFlatStatBonus(const FHeldItemEffect& Effect, const FString& StatName, int32 BaseStatValue, int32& NewStatValue);

	/** 1.0 (neutral) unless EffectType is DamageBoostWithRecoil. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|HeldItem")
	static float GetDamageMultiplier(const FHeldItemEffect& Effect);

	/** 0 unless EffectType is DamageBoostWithRecoil and DamageDealt > 0; otherwise floored at 1. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|HeldItem")
	static int32 CalculateHeldItemRecoil(const FHeldItemEffect& Effect, int32 DamageDealt);
};
