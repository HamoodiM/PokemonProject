#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PokemonStatusEffects.generated.h"

/**
 * Pure C++ per-turn status condition effects (Sleep, Freeze, Paralysis, Burn, Poison).
 * Status conditions themselves are still represented as FString on the Blueprint side
 * (S_BattleStatus.Status Condtion), matching UPokemonBattleLibrary's convention of
 * taking broken-out primitives rather than the project's UserDefinedStructs.
 *
 * A status's duration/thaw-roll state is tracked in a caller-owned StatusCounter int,
 * meaning: turns remaining for Sleep, unused (0) for the other conditions.
 */
UCLASS()
class POKEMONPROJECT_API UPokemonStatusEffects : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Rolls the initial duration for a newly-applied status. Only Sleep uses this
	 * (random 1-3 turns asleep); every other condition has no duration counter.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Status")
	static int32 RollInitialStatusCounter(const FString& StatusCondition);

	/**
	 * Resolves the start-of-turn status check: whether the Pokemon can act this turn,
	 * whether the status just wore off (Sleep waking / Freeze thawing), and the updated
	 * counter to persist. Call before move execution; if bCanAct is false, skip the move
	 * entirely (still consumes the turn).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Status")
	static bool ProcessTurnStartStatus(const FString& StatusCondition, int32 StatusCounter,
		int32& NewStatusCounter, FString& NewStatusCondition, bool& bStatusJustCleared);

	/** Burn halves physical Attack; every other condition/category is unaffected (multiplier 1.0). */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Status")
	static float GetStatusAttackMultiplier(const FString& StatusCondition, const FString& MoveCategory);

	/** Paralysis cuts Speed to 1/4, matching the stat-stage-independent classic penalty. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Status")
	static float GetStatusSpeedMultiplier(const FString& StatusCondition);

	/** Paralysis also carries an accuracy penalty on top of the speed cut (used when paralysis doesn't fully prevent the move). */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Status")
	static float GetStatusAccuracyMultiplier(const FString& StatusCondition);

	/** Per-turn chip damage from Burn/Poison, floored at 1 if the condition applies and MaxHP > 0. Returns 0 for other conditions. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Status")
	static int32 GetStatusChipDamage(const FString& StatusCondition, int32 MaxHP);
};
