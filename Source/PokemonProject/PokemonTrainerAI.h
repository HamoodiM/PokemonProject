#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PokemonTrainerAI.generated.h"

/** One candidate move a trainer AI is choosing between, broken out as primitives like UPokemonBattleLibrary's params. */
USTRUCT(BlueprintType)
struct FTrainerAIMoveOption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|AI")
	FString MoveName;

	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|AI")
	FString MoveType;

	/** "Physical", "Special", or "Status". */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|AI")
	FString Category = TEXT("Physical");

	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|AI")
	int32 Power = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|AI")
	int32 Accuracy = 100;

	/** Higher resolves first; matches the "Quick Attack has +1 priority" style move-effect data. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|AI")
	int32 Priority = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|AI")
	int32 CurrentPP = 1;

	/** "None" (or empty) if this move doesn't inflict a status. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|AI")
	FString StatusToApply = TEXT("None");

	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|AI")
	int32 StatusChance = 0;
};

/**
 * C++ trainer move-selection AI. Stateless per decision — call SelectMoveIndex once per
 * trainer turn with the current battle snapshot broken out as primitives, same convention
 * as UPokemonBattleLibrary. Reuses UPokemonBattleLibrary::GetTypeEffectiveness/
 * GetStatStageMultiplier for scoring rather than duplicating the type chart or stage math.
 */
UCLASS()
class POKEMONPROJECT_API UPokemonTrainerAI : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Deterministic expected damage for AI scoring purposes: same formula as
	 * UPokemonBattleLibrary::CalculateDamage but with the 0.85-1.0 random roll fixed at its
	 * midpoint (0.925) instead of sampled, so identical inputs always score identically.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|AI")
	static int32 CalculateExpectedDamage(int32 AttackerLevel, int32 AttackerAttack, int32 AttackerSpAttack,
		int32 AttackerAttackStage, int32 AttackerSpAttackStage, const FString& AttackerType1, const FString& AttackerType2,
		int32 DefenderDefence, int32 DefenderSpDefence, int32 DefenderDefenceStage, int32 DefenderSpDefenceStage,
		const FString& DefenderType1, const FString& DefenderType2,
		const FString& MoveCategory, int32 MovePower, const FString& MoveType);

	/** Scores a single candidate move; exposed mainly so tests/tools can inspect individual scores, not just the final pick. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|AI")
	static float ScoreMoveOption(const FTrainerAIMoveOption& Move,
		int32 AttackerLevel, int32 AttackerAttack, int32 AttackerSpAttack,
		int32 AttackerAttackStage, int32 AttackerSpAttackStage, const FString& AttackerType1, const FString& AttackerType2,
		int32 DefenderDefence, int32 DefenderSpDefence, int32 DefenderDefenceStage, int32 DefenderSpDefenceStage,
		const FString& DefenderType1, const FString& DefenderType2,
		int32 DefenderCurrentHP, int32 DefenderMaxHP, const FString& DefenderStatusCondition);

	/**
	 * Picks the index into AvailableMoves the trainer should use this turn. Skips any move
	 * with CurrentPP <= 0. Returns -1 if no move is usable (caller should e.g. Struggle).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|AI")
	static int32 SelectMoveIndex(const TArray<FTrainerAIMoveOption>& AvailableMoves,
		int32 AttackerLevel, int32 AttackerAttack, int32 AttackerSpAttack,
		int32 AttackerAttackStage, int32 AttackerSpAttackStage, const FString& AttackerType1, const FString& AttackerType2,
		int32 DefenderDefence, int32 DefenderSpDefence, int32 DefenderDefenceStage, int32 DefenderSpDefenceStage,
		const FString& DefenderType1, const FString& DefenderType2,
		int32 DefenderCurrentHP, int32 DefenderMaxHP, const FString& DefenderStatusCondition);
};
