#include "PokemonTrainerAI.h"
#include "PokemonBattleLibrary.h"

namespace PokemonTrainerAIInternal
{
	constexpr float FixedRandomFactor = 0.925f; // midpoint of CalculateDamage's 0.85-1.0 roll
	constexpr float KnockoutBonus = 1000.0f;
	constexpr float KnockoutPriorityWeight = 50.0f;
	constexpr float TiebreakPriorityWeight = 5.0f;
	constexpr float StatusMoveBaseScore = 40.0f;
}

int32 UPokemonTrainerAI::CalculateExpectedDamage(int32 AttackerLevel, int32 AttackerAttack, int32 AttackerSpAttack,
	int32 AttackerAttackStage, int32 AttackerSpAttackStage, const FString& AttackerType1, const FString& AttackerType2,
	int32 DefenderDefence, int32 DefenderSpDefence, int32 DefenderDefenceStage, int32 DefenderSpDefenceStage,
	const FString& DefenderType1, const FString& DefenderType2,
	const FString& MoveCategory, int32 MovePower, const FString& MoveType)
{
	using namespace PokemonTrainerAIInternal;

	if (MoveCategory == TEXT("Status"))
	{
		return 0;
	}

	float Atk, Def;
	if (MoveCategory == TEXT("Physical"))
	{
		Atk = AttackerAttack * UPokemonBattleLibrary::GetStatStageMultiplier(AttackerAttackStage);
		Def = DefenderDefence * UPokemonBattleLibrary::GetStatStageMultiplier(DefenderDefenceStage);
	}
	else
	{
		Atk = AttackerSpAttack * UPokemonBattleLibrary::GetStatStageMultiplier(AttackerSpAttackStage);
		Def = DefenderSpDefence * UPokemonBattleLibrary::GetStatStageMultiplier(DefenderSpDefenceStage);
	}

	const float Base = FMath::FloorToFloat(FMath::FloorToFloat((((2.0f * AttackerLevel / 5.0f) + 2.0f) * MovePower * (Atk / Def)) / 50.0f) + 2.0f);

	const bool bStab = (MoveType == AttackerType1) || (MoveType == AttackerType2);
	const float Stab = bStab ? 1.5f : 1.0f;
	const float TypeEff1 = UPokemonBattleLibrary::GetTypeEffectiveness(MoveType, DefenderType1);
	const float TypeEff2 = UPokemonBattleLibrary::GetTypeEffectiveness(MoveType, DefenderType2);

	const int32 Damage = FMath::FloorToInt(Base * Stab * TypeEff1 * TypeEff2 * FixedRandomFactor);
	return FMath::Max(Damage, 1);
}

float UPokemonTrainerAI::ScoreMoveOption(const FTrainerAIMoveOption& Move,
	int32 AttackerLevel, int32 AttackerAttack, int32 AttackerSpAttack,
	int32 AttackerAttackStage, int32 AttackerSpAttackStage, const FString& AttackerType1, const FString& AttackerType2,
	int32 DefenderDefence, int32 DefenderSpDefence, int32 DefenderDefenceStage, int32 DefenderSpDefenceStage,
	const FString& DefenderType1, const FString& DefenderType2,
	int32 DefenderCurrentHP, int32 DefenderMaxHP, const FString& DefenderStatusCondition)
{
	using namespace PokemonTrainerAIInternal;

	if (Move.Category == TEXT("Status"))
	{
		// Only status-application moves are modeled this session (stat-boost moves score 0 —
		// no stat-stage-change scoring exists yet, see UPokemonBattleLibrary's move-effect scope).
		const bool bHasRealStatusEffect = !Move.StatusToApply.IsEmpty() && Move.StatusToApply != TEXT("None");
		if (bHasRealStatusEffect && DefenderStatusCondition == TEXT("None"))
		{
			const float HealthFactor = DefenderMaxHP > 0 ? static_cast<float>(DefenderCurrentHP) / DefenderMaxHP : 1.0f;
			return StatusMoveBaseScore * (FMath::Clamp(Move.StatusChance, 0, 100) / 100.0f) * HealthFactor;
		}
		return 0.0f;
	}

	const int32 ExpectedDamage = CalculateExpectedDamage(AttackerLevel, AttackerAttack, AttackerSpAttack,
		AttackerAttackStage, AttackerSpAttackStage, AttackerType1, AttackerType2,
		DefenderDefence, DefenderSpDefence, DefenderDefenceStage, DefenderSpDefenceStage,
		DefenderType1, DefenderType2, Move.Category, Move.Power, Move.MoveType);

	const float HitChance = FMath::Clamp(Move.Accuracy, 0, 100) / 100.0f;
	float Score = ExpectedDamage * HitChance;

	if (ExpectedDamage >= DefenderCurrentHP)
	{
		// Weight the KO bonus by hit chance too — a low-accuracy overkill move shouldn't
		// outscore a reliable lower-power finisher just because raw damage is higher.
		Score += HitChance * KnockoutBonus + Move.Priority * KnockoutPriorityWeight;
	}
	else
	{
		Score += Move.Priority * TiebreakPriorityWeight;
	}

	return Score;
}

int32 UPokemonTrainerAI::SelectMoveIndex(const TArray<FTrainerAIMoveOption>& AvailableMoves,
	int32 AttackerLevel, int32 AttackerAttack, int32 AttackerSpAttack,
	int32 AttackerAttackStage, int32 AttackerSpAttackStage, const FString& AttackerType1, const FString& AttackerType2,
	int32 DefenderDefence, int32 DefenderSpDefence, int32 DefenderDefenceStage, int32 DefenderSpDefenceStage,
	const FString& DefenderType1, const FString& DefenderType2,
	int32 DefenderCurrentHP, int32 DefenderMaxHP, const FString& DefenderStatusCondition)
{
	int32 BestIndex = -1;
	float BestScore = -1.0f;

	for (int32 Index = 0; Index < AvailableMoves.Num(); ++Index)
	{
		const FTrainerAIMoveOption& Move = AvailableMoves[Index];
		if (Move.CurrentPP <= 0)
		{
			continue;
		}

		const float Score = ScoreMoveOption(Move, AttackerLevel, AttackerAttack, AttackerSpAttack,
			AttackerAttackStage, AttackerSpAttackStage, AttackerType1, AttackerType2,
			DefenderDefence, DefenderSpDefence, DefenderDefenceStage, DefenderSpDefenceStage,
			DefenderType1, DefenderType2, DefenderCurrentHP, DefenderMaxHP, DefenderStatusCondition);

		if (BestIndex == -1 || Score > BestScore)
		{
			BestIndex = Index;
			BestScore = Score;
		}
	}

	return BestIndex;
}
