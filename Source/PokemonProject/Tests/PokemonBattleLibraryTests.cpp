#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../PokemonBattleLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonTypeEffectivenessTest, "Pokemon.Battle.TypeEffectiveness",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonTypeEffectivenessTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Fire is super effective against Grass"), UPokemonBattleLibrary::GetTypeEffectiveness(TEXT("Fire"), TEXT("Grass")), 2.0f);
	TestEqual(TEXT("Fire is not very effective against Water"), UPokemonBattleLibrary::GetTypeEffectiveness(TEXT("Fire"), TEXT("Water")), 0.5f);
	TestEqual(TEXT("Water is super effective against Fire"), UPokemonBattleLibrary::GetTypeEffectiveness(TEXT("Water"), TEXT("Fire")), 2.0f);
	TestEqual(TEXT("Grass is super effective against Water"), UPokemonBattleLibrary::GetTypeEffectiveness(TEXT("Grass"), TEXT("Water")), 2.0f);
	TestEqual(TEXT("Grass is not very effective against Fire"), UPokemonBattleLibrary::GetTypeEffectiveness(TEXT("Grass"), TEXT("Fire")), 0.5f);
	TestEqual(TEXT("Unrelated types are neutral"), UPokemonBattleLibrary::GetTypeEffectiveness(TEXT("Normal"), TEXT("Ghost")), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonStatStageMultiplierTest, "Pokemon.Battle.StatStageMultiplier",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonStatStageMultiplierTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Stage 0 is neutral"), UPokemonBattleLibrary::GetStatStageMultiplier(0), 1.0f);
	TestEqual(TEXT("Stage +6 is 4x"), UPokemonBattleLibrary::GetStatStageMultiplier(6), 4.0f);
	TestEqual(TEXT("Stage -6 is 0.25x"), UPokemonBattleLibrary::GetStatStageMultiplier(-6), 0.25f);
	TestEqual(TEXT("Accuracy stage 0 is neutral"), UPokemonBattleLibrary::GetAccuracyStageMultiplier(0), 1.0f);
	TestEqual(TEXT("Accuracy stage +6 is 4x"), UPokemonBattleLibrary::GetAccuracyStageMultiplier(6), 4.0f);
	TestEqual(TEXT("Accuracy stage -6 is 0.25x"), UPokemonBattleLibrary::GetAccuracyStageMultiplier(-6), 0.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonClampStatStageTest, "Pokemon.Battle.ClampStatStage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonClampStatStageTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Clamps above +6"), UPokemonBattleLibrary::ClampStatStage(5, 5), 6);
	TestEqual(TEXT("Clamps below -6"), UPokemonBattleLibrary::ClampStatStage(-5, -5), -6);
	TestEqual(TEXT("Passes through within range"), UPokemonBattleLibrary::ClampStatStage(1, 2), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonScaleStatTest, "Pokemon.Battle.ScaleStat",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonScaleStatTest::RunTest(const FString& Parameters)
{
	// HP formula: floor(Base*Level/50) + Level + 10
	TestEqual(TEXT("HP scaling at level 50"), UPokemonBattleLibrary::ScaleStat(45, 50, true), 45 + 50 + 10);
	// Non-HP formula: floor(Base*Level/50) + 5
	TestEqual(TEXT("Non-HP scaling at level 50"), UPokemonBattleLibrary::ScaleStat(45, 50, false), 45 + 5);
	TestEqual(TEXT("Non-HP scaling at level 1 floors toward zero contribution"), UPokemonBattleLibrary::ScaleStat(45, 1, false), 0 + 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonCalculateDamageTest, "Pokemon.Battle.CalculateDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonCalculateDamageTest::RunTest(const FString& Parameters)
{
	// Status moves never deal direct damage.
	const int32 StatusDamage = UPokemonBattleLibrary::CalculateDamage(
		10, 20, 20, 0, 0, TEXT("Fire"), TEXT(""), 20, 20, 0, 0, TEXT("Normal"), TEXT(""), TEXT("Status"), 0, TEXT("Fire"));
	TestEqual(TEXT("Status move deals 0 damage"), StatusDamage, 0);

	// Level 10 attacker, 20 Attack, 40-power STAB Physical move, level-matched 20 Defence defender,
	// no type advantage either way. Base damage before the 0.85-1.0 random factor is exactly 6
	// (floor(floor((((2*10/5)+2)*40*(20/20))/50)+2) = floor(4.8)+2 = 6), times 1.5 STAB = 9 max, 7.65 -> 7 min.
	const int32 Damage = UPokemonBattleLibrary::CalculateDamage(
		10, 20, 20, 0, 0, TEXT("Fire"), TEXT(""), 20, 20, 0, 0, TEXT("Normal"), TEXT(""), TEXT("Physical"), 40, TEXT("Fire"));
	TestTrue(TEXT("Damage falls within the expected random-roll range"), Damage >= 7 && Damage <= 9);

	// Damage is always at least 1, even for a lopsided matchup in the defender's favor.
	const int32 MinDamage = UPokemonBattleLibrary::CalculateDamage(
		1, 1, 1, -6, -6, TEXT("Grass"), TEXT(""), 999, 999, 6, 6, TEXT("Fire"), TEXT(""), TEXT("Physical"), 1, TEXT("Grass"));
	TestTrue(TEXT("Damage is clamped to a minimum of 1"), MinDamage >= 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonComputeLevelUpTest, "Pokemon.Battle.ComputeLevelUp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonComputeLevelUpTest::RunTest(const FString& Parameters)
{
	{
		// Level 5 needs 125 EXP (5^3); 65 is short of that, so no level-up.
		int32 NewLevel, NewEXP; bool bLeveledUp;
		UPokemonBattleLibrary::ComputeLevelUp(4, 100, 60, 5, NewLevel, NewEXP, bLeveledUp);
		TestEqual(TEXT("No level-up just under threshold: level unchanged"), NewLevel, 4);
		TestEqual(TEXT("No level-up just under threshold: EXP still accumulates"), NewEXP, 65);
		TestFalse(TEXT("No level-up just under threshold: bLeveledUp is false"), bLeveledUp);
	}
	{
		// 130 EXP clears the level-5 threshold (125) but not level-6 (216).
		int32 NewLevel, NewEXP; bool bLeveledUp;
		UPokemonBattleLibrary::ComputeLevelUp(4, 100, 120, 10, NewLevel, NewEXP, bLeveledUp);
		TestEqual(TEXT("Single level-up reaches the expected level"), NewLevel, 5);
		TestTrue(TEXT("Single level-up sets bLeveledUp"), bLeveledUp);
	}
	{
		// 1000 EXP from level 1 clears every cubic threshold up to level 10 (10^3 == 1000) in one award.
		int32 NewLevel, NewEXP; bool bLeveledUp;
		UPokemonBattleLibrary::ComputeLevelUp(1, 100, 0, 1000, NewLevel, NewEXP, bLeveledUp);
		TestEqual(TEXT("Multi level-up from one large EXP award"), NewLevel, 10);
		TestTrue(TEXT("Multi level-up sets bLeveledUp"), bLeveledUp);
	}
	{
		// MaxLevel caps advancement even if EXP would justify going further.
		int32 NewLevel, NewEXP; bool bLeveledUp;
		UPokemonBattleLibrary::ComputeLevelUp(4, 5, 1000000, 0, NewLevel, NewEXP, bLeveledUp);
		TestEqual(TEXT("Level-up is capped at MaxLevel"), NewLevel, 5);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonApplyLevelUpHPTest, "Pokemon.Battle.ApplyLevelUpHP",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonApplyLevelUpHPTest::RunTest(const FString& Parameters)
{
	// Full HP before level-up stays capped at the new max, not exceeding it.
	TestEqual(TEXT("Full HP before level-up caps at new max"), UPokemonBattleLibrary::ApplyLevelUpHP(20, 20, 25), 25);
	// Partial HP preserves the amount of damage taken, not the raw HP value.
	TestEqual(TEXT("Partial HP preserves damage taken"), UPokemonBattleLibrary::ApplyLevelUpHP(5, 20, 25), 10);
	// Never drops below zero even in a pathological shrinking-max case.
	TestEqual(TEXT("HP never goes negative"), UPokemonBattleLibrary::ApplyLevelUpHP(0, 20, 15), 0);
	return true;
}

/**
 * Drives a full mock battle headlessly through the ported C++ battle functions only
 * (no Actor/UObject/level dependencies), mirroring WBP_BattleMenu_Recovered.ResolveMove's
 * turn sequence: roll accuracy, calculate damage, subtract HP, repeat until one side faints.
 */
BEGIN_DEFINE_SPEC(FPokemonMockBattleSpec, "Pokemon.Battle.MockBattle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FPokemonMockBattleSpec)

void FPokemonMockBattleSpec::Define()
{
	It("runs a full mock battle to completion without going negative", [this]()
	{
		struct FMockCombatant
		{
			int32 Level;
			int32 Attack;
			int32 Defence;
			FString Type1;
			int32 HP;
			int32 MaxHP;
		};

		FMockCombatant Player{ 8, 18, 15, TEXT("Water"), 0, 0 };
		Player.MaxHP = Player.HP = UPokemonBattleLibrary::ScaleStat(35, Player.Level, true);

		FMockCombatant Enemy{ 7, 16, 14, TEXT("Grass"), 0, 0 };
		Enemy.MaxHP = Enemy.HP = UPokemonBattleLibrary::ScaleStat(30, Enemy.Level, true);

		constexpr int32 MoveAccuracy = 100; // deterministic hits so the battle can't stall on misses
		constexpr int32 MovePower = 35;
		constexpr int32 MaxTurns = 200; // safety cap so a pathological RNG sequence can't hang the test

		bool bPlayerWon = false;
		bool bEnemyWon = false;
		int32 Turn = 0;

		for (; Turn < MaxTurns; ++Turn)
		{
			// Player's turn.
			if (UPokemonBattleLibrary::RollAccuracy(MoveAccuracy, 0, 0))
			{
				const int32 Damage = UPokemonBattleLibrary::CalculateDamage(
					Player.Level, Player.Attack, Player.Attack, 0, 0, Player.Type1, TEXT(""),
					Enemy.Defence, Enemy.Defence, 0, 0, Enemy.Type1, TEXT(""),
					TEXT("Physical"), MovePower, Player.Type1);
				Enemy.HP = FMath::Clamp(Enemy.HP - Damage, 0, Enemy.MaxHP);
			}
			if (Enemy.HP <= 0)
			{
				bPlayerWon = true;
				break;
			}

			// Enemy's turn.
			if (UPokemonBattleLibrary::RollAccuracy(MoveAccuracy, 0, 0))
			{
				const int32 Damage = UPokemonBattleLibrary::CalculateDamage(
					Enemy.Level, Enemy.Attack, Enemy.Attack, 0, 0, Enemy.Type1, TEXT(""),
					Player.Defence, Player.Defence, 0, 0, Player.Type1, TEXT(""),
					TEXT("Physical"), MovePower, Enemy.Type1);
				Player.HP = FMath::Clamp(Player.HP - Damage, 0, Player.MaxHP);
			}
			if (Player.HP <= 0)
			{
				bEnemyWon = true;
				break;
			}
		}

		TestTrue(TEXT("Battle reached a winner within the turn cap"), bPlayerWon || bEnemyWon);
		TestFalse(TEXT("Both sides did not somehow lose simultaneously"), bPlayerWon && bEnemyWon);
		TestTrue(TEXT("Enemy HP never went negative"), Enemy.HP >= 0);
		TestTrue(TEXT("Player HP never went negative"), Player.HP >= 0);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
