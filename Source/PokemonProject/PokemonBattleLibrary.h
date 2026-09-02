#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PokemonBattleLibrary.generated.h"

/**
 * Pure C++ port of the Pokemon battle/EXP/stat formulas that previously lived in the
 * Blueprint function library BPFL_Battle. Functions take broken-out primitive parameters
 * rather than the project's Blueprint UserDefinedStructs (S_PokemonStats, S_TMMove, ...)
 * so no Blueprint-side struct has to change type to use these.
 */
UCLASS()
class POKEMONPROJECT_API UPokemonBattleLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Battle")
	static float GetTypeEffectiveness(const FString& AttackType, const FString& DefendType);

	UFUNCTION(BlueprintCallable, Category = "Pokemon|Battle")
	static float GetStatStageMultiplier(int32 Stage);

	UFUNCTION(BlueprintCallable, Category = "Pokemon|Battle")
	static float GetAccuracyStageMultiplier(int32 Stage);

	/** Applies the level-scaling formula for one stat. bIsHP selects the HP formula, otherwise the shared non-HP formula. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Battle")
	static int32 ScaleStat(int32 BaseStat, int32 Level, bool bIsHP);

	UFUNCTION(BlueprintCallable, Category = "Pokemon|Battle")
	static bool RollAccuracy(int32 MoveAccuracy, int32 AttackerAccuracyStage, int32 DefenderEvasionStage);

	UFUNCTION(BlueprintCallable, Category = "Pokemon|Battle")
	static int32 CalculateDamage(int32 AttackerLevel, int32 AttackerAttack, int32 AttackerSpAttack,
		int32 AttackerAttackStage, int32 AttackerSpAttackStage, const FString& AttackerType1, const FString& AttackerType2,
		int32 DefenderDefence, int32 DefenderSpDefence, int32 DefenderDefenceStage, int32 DefenderSpDefenceStage,
		const FString& DefenderType1, const FString& DefenderType2,
		const FString& MoveCategory, int32 MovePower, const FString& MoveType);

	UFUNCTION(BlueprintCallable, Category = "Pokemon|Battle")
	static int32 ClampStatStage(int32 CurrentStage, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Pokemon|Battle")
	static bool ShouldApplyStatus(const FString& CurrentStatus, int32 Chance);

	UFUNCTION(BlueprintCallable, Category = "Pokemon|Battle")
	static bool CalculateCatchChance(int32 BaseCatchRate, int32 HP, int32 MaxHP, const FString& StatusCondition, float BallBonus);

	/** Cubic (level^3) medium-fast EXP threshold. Advances NewLevel/NewEXP as far as CurrentEXP + ExpGained allows, capped at MaxLevel. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Battle")
	static void ComputeLevelUp(int32 CurrentLevel, int32 MaxLevel, int32 CurrentEXP, int32 ExpGained,
		int32& NewLevel, int32& NewEXP, bool& bLeveledUp);

	/** Preserved-damage-delta HP formula used on level-up: keeps the same damage taken, clamped to the new max. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Battle")
	static int32 ApplyLevelUpHP(int32 OldHP, int32 OldMaxHP, int32 NewMaxHP);
};
