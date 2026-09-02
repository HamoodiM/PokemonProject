#include "PokemonMoveEffects.h"
#include "PokemonBattleLibrary.h"
#include "PokemonStatusEffects.h"

bool UPokemonMoveEffectLibrary::ResolveStatChange(const FMoveEffect& Effect, int32 CurrentStage, int32& NewStage)
{
	if (Effect.EffectType != EMoveEffectType::StatChange)
	{
		NewStage = CurrentStage;
		return false;
	}

	NewStage = UPokemonBattleLibrary::ClampStatStage(CurrentStage, Effect.StatStageDelta);
	return true;
}

bool UPokemonMoveEffectLibrary::ResolveStatusApplication(const FMoveEffect& Effect, const FString& CurrentTargetStatus,
	FString& NewStatus, int32& NewStatusCounter)
{
	NewStatus = CurrentTargetStatus;
	NewStatusCounter = 0;

	if (Effect.EffectType != EMoveEffectType::ApplyStatus)
	{
		return false;
	}

	if (!UPokemonBattleLibrary::ShouldApplyStatus(CurrentTargetStatus, Effect.StatusChance))
	{
		return false;
	}

	NewStatus = Effect.StatusToApply;
	NewStatusCounter = UPokemonStatusEffects::RollInitialStatusCounter(NewStatus);
	return true;
}

int32 UPokemonMoveEffectLibrary::CalculateRecoilDamage(const FMoveEffect& Effect, int32 DamageDealt)
{
	if (Effect.EffectType != EMoveEffectType::Recoil || DamageDealt <= 0)
	{
		return 0;
	}

	return FMath::Max(FMath::CeilToInt(DamageDealt * Effect.RecoilFraction), 1);
}

int32 UPokemonMoveEffectLibrary::CalculateHealAmount(const FMoveEffect& Effect, int32 MaxHP)
{
	if (Effect.EffectType != EMoveEffectType::Heal || MaxHP <= 0)
	{
		return 0;
	}

	return FMath::Max(FMath::FloorToInt(MaxHP * Effect.HealFraction), 1);
}

void UPokemonMoveEffectLibrary::ProcessTwoTurnMove(bool bCurrentlyCharging, bool& bShouldExecuteDamageThisTurn, bool& bNewChargingState)
{
	if (bCurrentlyCharging)
	{
		bShouldExecuteDamageThisTurn = true;
		bNewChargingState = false;
	}
	else
	{
		bShouldExecuteDamageThisTurn = false;
		bNewChargingState = true;
	}
}
