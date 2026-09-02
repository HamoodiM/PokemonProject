#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PokemonMoveEffects.generated.h"

UENUM(BlueprintType)
enum class EMoveEffectType : uint8
{
	None,
	StatChange,
	ApplyStatus,
	Recoil,
	Heal,
	TwoTurnCharge
};

/**
 * Data-driven description of a move's secondary effect, separate from its raw damage/accuracy
 * (which stays in the project's existing move data). One move = one FMoveEffect; a move with no
 * secondary effect just uses EffectType::None. Broken-out primitives, same convention as
 * UPokemonBattleLibrary/UPokemonTrainerAI — no dependency on the project's UserDefinedStructs.
 */
USTRUCT(BlueprintType)
struct FMoveEffect
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|MoveEffect")
	EMoveEffectType EffectType = EMoveEffectType::None;

	/** StatChange only: "Attack", "Defence", "SpAttack", "SpDefence", "Speed", "Accuracy", "Evasion". */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|MoveEffect")
	FString TargetStat;

	/** StatChange only: signed stage delta, e.g. +2 for Swords Dance, +1 for Withdraw. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|MoveEffect")
	int32 StatStageDelta = 0;

	/** StatChange only: true for self-buffs (Swords Dance), false for target-debuffs. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|MoveEffect")
	bool bAppliesToSelf = true;

	/** ApplyStatus only: "None" if this effect doesn't apply one. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|MoveEffect")
	FString StatusToApply = TEXT("None");

	/** ApplyStatus only, 0-100. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|MoveEffect")
	int32 StatusChance = 100;

	/** Recoil only: fraction of damage dealt the attacker takes back, e.g. 1/3 for Double-Edge. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|MoveEffect")
	float RecoilFraction = 0.0f;

	/** Heal only: fraction of the healer's MaxHP restored, e.g. 1.0 for Recover. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|MoveEffect")
	float HealFraction = 0.0f;
};

/**
 * Pure C++ resolvers for FMoveEffect, one function per effect category. Reuses
 * UPokemonBattleLibrary::ClampStatStage/ShouldApplyStatus and
 * UPokemonStatusEffects::RollInitialStatusCounter rather than duplicating that logic, so a move
 * effect stays consistent with the existing stat-stage/status systems.
 */
UCLASS()
class POKEMONPROJECT_API UPokemonMoveEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** No-op (returns false, NewStage=CurrentStage) unless EffectType is StatChange. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|MoveEffect")
	static bool ResolveStatChange(const FMoveEffect& Effect, int32 CurrentStage, int32& NewStage);

	/** No-op (returns false) unless EffectType is ApplyStatus. Rolls the chance and, on success, the new status's initial counter. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|MoveEffect")
	static bool ResolveStatusApplication(const FMoveEffect& Effect, const FString& CurrentTargetStatus,
		FString& NewStatus, int32& NewStatusCounter);

	/** 0 unless EffectType is Recoil or DamageDealt <= 0; otherwise floored at 1 so recoil is never zero when it should apply. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|MoveEffect")
	static int32 CalculateRecoilDamage(const FMoveEffect& Effect, int32 DamageDealt);

	/** 0 unless EffectType is Heal; otherwise floored at 1 (caller should still clamp to MaxHP). */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|MoveEffect")
	static int32 CalculateHealAmount(const FMoveEffect& Effect, int32 MaxHP);

	/**
	 * Two-turn moves (Solar Beam/Dive/Fly-style): call once per turn a Pokemon uses such a move.
	 * Not charging -> begins charging, no damage this turn. Charging -> releases the attack this
	 * turn and clears the charging state. Caller owns the persisted bCurrentlyCharging flag.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|MoveEffect")
	static void ProcessTwoTurnMove(bool bCurrentlyCharging, bool& bShouldExecuteDamageThisTurn, bool& bNewChargingState);
};
