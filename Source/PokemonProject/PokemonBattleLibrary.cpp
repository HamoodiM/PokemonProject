#include "PokemonBattleLibrary.h"

float UPokemonBattleLibrary::GetTypeEffectiveness(const FString& AttackType, const FString& DefendType)
{
	// Fire/Water/Grass triangle only — matches BPFL_Battle, everything else is neutral (1.0).
	if (AttackType == TEXT("Fire"))
	{
		if (DefendType == TEXT("Grass")) return 2.0f;
		if (DefendType == TEXT("Water")) return 0.5f;
	}
	else if (AttackType == TEXT("Water"))
	{
		if (DefendType == TEXT("Fire")) return 2.0f;
		if (DefendType == TEXT("Grass")) return 0.5f;
	}
	else if (AttackType == TEXT("Grass"))
	{
		if (DefendType == TEXT("Water")) return 2.0f;
		if (DefendType == TEXT("Fire")) return 0.5f;
	}
	return 1.0f;
}

float UPokemonBattleLibrary::GetStatStageMultiplier(int32 Stage)
{
	return Stage >= 0 ? (2.0f + Stage) / 2.0f : 2.0f / (2.0f - Stage);
}

float UPokemonBattleLibrary::GetAccuracyStageMultiplier(int32 Stage)
{
	return Stage >= 0 ? (2.0f + Stage) / 2.0f : 2.0f / (2.0f - Stage);
}

int32 UPokemonBattleLibrary::ScaleStat(int32 BaseStat, int32 Level, bool bIsHP)
{
	const int32 Scaled = FMath::FloorToInt(BaseStat * Level / 50.0f);
	return bIsHP ? Scaled + Level + 10 : Scaled + 5;
}

bool UPokemonBattleLibrary::RollAccuracy(int32 MoveAccuracy, int32 AttackerAccuracyStage, int32 DefenderEvasionStage)
{
	const float Threshold = (MoveAccuracy * GetAccuracyStageMultiplier(AttackerAccuracyStage)) / GetAccuracyStageMultiplier(DefenderEvasionStage);
	return FMath::FRandRange(0.0f, 100.0f) <= Threshold;
}

int32 UPokemonBattleLibrary::CalculateDamage(int32 AttackerLevel, int32 AttackerAttack, int32 AttackerSpAttack,
	int32 AttackerAttackStage, int32 AttackerSpAttackStage, const FString& AttackerType1, const FString& AttackerType2,
	int32 DefenderDefence, int32 DefenderSpDefence, int32 DefenderDefenceStage, int32 DefenderSpDefenceStage,
	const FString& DefenderType1, const FString& DefenderType2,
	const FString& MoveCategory, int32 MovePower, const FString& MoveType)
{
	if (MoveCategory == TEXT("Status"))
	{
		return 0;
	}

	float Atk, Def;
	if (MoveCategory == TEXT("Physical"))
	{
		Atk = AttackerAttack * GetStatStageMultiplier(AttackerAttackStage);
		Def = DefenderDefence * GetStatStageMultiplier(DefenderDefenceStage);
	}
	else
	{
		Atk = AttackerSpAttack * GetStatStageMultiplier(AttackerSpAttackStage);
		Def = DefenderSpDefence * GetStatStageMultiplier(DefenderSpDefenceStage);
	}

	const float Base = FMath::FloorToFloat(FMath::FloorToFloat((((2.0f * AttackerLevel / 5.0f) + 2.0f) * MovePower * (Atk / Def)) / 50.0f) + 2.0f);

	const bool bStab = (MoveType == AttackerType1) || (MoveType == AttackerType2);
	const float Stab = bStab ? 1.5f : 1.0f;
	const float TypeEff1 = GetTypeEffectiveness(MoveType, DefenderType1);
	const float TypeEff2 = GetTypeEffectiveness(MoveType, DefenderType2);
	const float RandomFactor = FMath::FRandRange(0.85f, 1.0f);

	const int32 Damage = FMath::FloorToInt(Base * Stab * TypeEff1 * TypeEff2 * RandomFactor);
	return FMath::Max(Damage, 1);
}

int32 UPokemonBattleLibrary::ClampStatStage(int32 CurrentStage, int32 Amount)
{
	return FMath::Clamp(CurrentStage + Amount, -6, 6);
}

bool UPokemonBattleLibrary::ShouldApplyStatus(const FString& CurrentStatus, int32 Chance)
{
	return CurrentStatus == TEXT("None") && FMath::FRandRange(0.0f, 100.0f) <= Chance;
}

bool UPokemonBattleLibrary::CalculateCatchChance(int32 BaseCatchRate, int32 HP, int32 MaxHP, const FString& StatusCondition, float BallBonus)
{
	float StatusMult = 1.0f;
	if (StatusCondition == TEXT("Sleep") || StatusCondition == TEXT("Freeze"))
	{
		StatusMult = 2.5f;
	}
	else if (StatusCondition == TEXT("Paralysis") || StatusCondition == TEXT("Poison") || StatusCondition == TEXT("Burn"))
	{
		StatusMult = 1.5f;
	}

	const float A = ((3.0f * MaxHP - 2.0f * HP) * BaseCatchRate * BallBonus) / (3.0f * MaxHP);
	const float B = 65536.0f / FMath::Pow(255.0f / FMath::Min(A * StatusMult, 255.0f), 0.1875f);

	for (int32 i = 0; i < 4; ++i)
	{
		if (FMath::RandRange(0, 65535) >= B)
		{
			return false;
		}
	}
	return true;
}

void UPokemonBattleLibrary::ComputeLevelUp(int32 CurrentLevel, int32 MaxLevel, int32 CurrentEXP, int32 ExpGained,
	int32& NewLevel, int32& NewEXP, bool& bLeveledUp)
{
	NewEXP = CurrentEXP + ExpGained;
	NewLevel = CurrentLevel;

	for (int32 Level = CurrentLevel + 1; Level <= MaxLevel; ++Level)
	{
		if (NewEXP >= Level * Level * Level)
		{
			NewLevel = Level;
		}
		else
		{
			break;
		}
	}

	bLeveledUp = NewLevel > CurrentLevel;
}

int32 UPokemonBattleLibrary::ApplyLevelUpHP(int32 OldHP, int32 OldMaxHP, int32 NewMaxHP)
{
	return FMath::Clamp(OldHP + (NewMaxHP - OldMaxHP), 0, NewMaxHP);
}
