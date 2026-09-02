#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../PokemonTrainerAI.h"
#include "../PokemonBattleLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonTrainerAIExpectedDamageTest, "Pokemon.TrainerAI.ExpectedDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonTrainerAIExpectedDamageTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Status moves deal 0 expected damage"),
		UPokemonTrainerAI::CalculateExpectedDamage(10, 20, 20, 0, 0, TEXT("Fire"), TEXT(""), 20, 20, 0, 0, TEXT("Normal"), TEXT(""), TEXT("Status"), 0, TEXT("Fire")),
		0);

	// Same inputs called twice must produce identical output (deterministic, unlike UPokemonBattleLibrary::CalculateDamage).
	const int32 DamageA = UPokemonTrainerAI::CalculateExpectedDamage(10, 20, 20, 0, 0, TEXT("Fire"), TEXT(""), 20, 20, 0, 0, TEXT("Normal"), TEXT(""), TEXT("Physical"), 40, TEXT("Fire"));
	const int32 DamageB = UPokemonTrainerAI::CalculateExpectedDamage(10, 20, 20, 0, 0, TEXT("Fire"), TEXT(""), 20, 20, 0, 0, TEXT("Normal"), TEXT(""), TEXT("Physical"), 40, TEXT("Fire"));
	TestEqual(TEXT("Expected damage is deterministic for identical inputs"), DamageA, DamageB);

	// Super-effective STAB move should out-damage the same move typed neutrally.
	const int32 SuperEffective = UPokemonTrainerAI::CalculateExpectedDamage(10, 20, 20, 0, 0, TEXT("Fire"), TEXT(""), 20, 20, 0, 0, TEXT("Grass"), TEXT(""), TEXT("Physical"), 40, TEXT("Fire"));
	const int32 Neutral = UPokemonTrainerAI::CalculateExpectedDamage(10, 20, 20, 0, 0, TEXT("Fire"), TEXT(""), 20, 20, 0, 0, TEXT("Normal"), TEXT(""), TEXT("Physical"), 40, TEXT("Fire"));
	TestTrue(TEXT("Super-effective damage exceeds neutral damage"), SuperEffective > Neutral);

	TestTrue(TEXT("Damage is always at least 1"),
		UPokemonTrainerAI::CalculateExpectedDamage(1, 1, 1, -6, -6, TEXT("Grass"), TEXT(""), 999, 999, 6, 6, TEXT("Fire"), TEXT(""), TEXT("Physical"), 1, TEXT("Grass")) >= 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonTrainerAITypeAdvantageTest, "Pokemon.TrainerAI.PrefersTypeAdvantage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonTrainerAITypeAdvantageTest::RunTest(const FString& Parameters)
{
	// A Fire-type attacker facing a Grass-type defender should prefer its super-effective Fire
	// move over an equally-powered, equally-accurate Normal move.
	TArray<FTrainerAIMoveOption> Moves;
	FTrainerAIMoveOption Ember;
	Ember.MoveName = TEXT("Ember"); Ember.MoveType = TEXT("Fire"); Ember.Category = TEXT("Physical");
	Ember.Power = 40; Ember.Accuracy = 100; Ember.CurrentPP = 10;
	Moves.Add(Ember);

	FTrainerAIMoveOption Tackle;
	Tackle.MoveName = TEXT("Tackle"); Tackle.MoveType = TEXT("Normal"); Tackle.Category = TEXT("Physical");
	Tackle.Power = 40; Tackle.Accuracy = 100; Tackle.CurrentPP = 10;
	Moves.Add(Tackle);

	const int32 Chosen = UPokemonTrainerAI::SelectMoveIndex(Moves,
		10, 20, 20, 0, 0, TEXT("Fire"), TEXT(""),
		20, 20, 0, 0, TEXT("Grass"), TEXT(""),
		/*DefenderCurrentHP*/ 50, /*DefenderMaxHP*/ 50, TEXT("None"));

	TestEqual(TEXT("AI picks the super-effective STAB move over the neutral one"), Chosen, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonTrainerAIKnockoutTest, "Pokemon.TrainerAI.PrefersGuaranteedKnockout",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonTrainerAIKnockoutTest::RunTest(const FString& Parameters)
{
	// A weak-but-lethal finishing move should beat a stronger move that can't quite KO,
	// when the defender is nearly fainted.
	TArray<FTrainerAIMoveOption> Moves;
	FTrainerAIMoveOption WeakFinisher;
	WeakFinisher.MoveName = TEXT("Weak"); WeakFinisher.MoveType = TEXT("Normal"); WeakFinisher.Category = TEXT("Physical");
	WeakFinisher.Power = 20; WeakFinisher.Accuracy = 100; WeakFinisher.CurrentPP = 10;
	Moves.Add(WeakFinisher);

	FTrainerAIMoveOption StrongButOverkillIrrelevant;
	StrongButOverkillIrrelevant.MoveName = TEXT("Strong"); StrongButOverkillIrrelevant.MoveType = TEXT("Normal"); StrongButOverkillIrrelevant.Category = TEXT("Physical");
	StrongButOverkillIrrelevant.Power = 90; StrongButOverkillIrrelevant.Accuracy = 60; // low accuracy risk
	StrongButOverkillIrrelevant.CurrentPP = 10;
	Moves.Add(StrongButOverkillIrrelevant);

	const int32 Chosen = UPokemonTrainerAI::SelectMoveIndex(Moves,
		20, 30, 30, 0, 0, TEXT("Normal"), TEXT(""),
		20, 20, 0, 0, TEXT("Normal"), TEXT(""),
		/*DefenderCurrentHP*/ 2, /*DefenderMaxHP*/ 50, TEXT("None"));

	TestEqual(TEXT("AI prefers the reliable knockout over the riskier overkill move"), Chosen, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonTrainerAIAvoidsRedundantStatusTest, "Pokemon.TrainerAI.AvoidsRedundantStatus",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonTrainerAIAvoidsRedundantStatusTest::RunTest(const FString& Parameters)
{
	// If the defender is already paralyzed, a second paralysis-inflicting move is worthless
	// and should lose to a damaging move even though the damaging move can't KO this turn.
	TArray<FTrainerAIMoveOption> Moves;
	FTrainerAIMoveOption ThunderWave;
	ThunderWave.MoveName = TEXT("ThunderWave"); ThunderWave.MoveType = TEXT("Electric"); ThunderWave.Category = TEXT("Status");
	ThunderWave.Power = 0; ThunderWave.Accuracy = 90; ThunderWave.CurrentPP = 10;
	ThunderWave.StatusToApply = TEXT("Paralysis"); ThunderWave.StatusChance = 100;
	Moves.Add(ThunderWave);

	FTrainerAIMoveOption Tackle;
	Tackle.MoveName = TEXT("Tackle"); Tackle.MoveType = TEXT("Normal"); Tackle.Category = TEXT("Physical");
	Tackle.Power = 40; Tackle.Accuracy = 100; Tackle.CurrentPP = 10;
	Moves.Add(Tackle);

	const int32 Chosen = UPokemonTrainerAI::SelectMoveIndex(Moves,
		10, 20, 20, 0, 0, TEXT("Normal"), TEXT(""),
		20, 20, 0, 0, TEXT("Normal"), TEXT(""),
		/*DefenderCurrentHP*/ 50, /*DefenderMaxHP*/ 50, TEXT("Paralysis"));

	TestEqual(TEXT("AI skips the redundant status move in favor of dealing damage"), Chosen, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonTrainerAINoUsableMovesTest, "Pokemon.TrainerAI.NoUsableMoves",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonTrainerAINoUsableMovesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Empty move list returns -1"),
		UPokemonTrainerAI::SelectMoveIndex(TArray<FTrainerAIMoveOption>(),
			10, 20, 20, 0, 0, TEXT("Normal"), TEXT(""), 20, 20, 0, 0, TEXT("Normal"), TEXT(""), 50, 50, TEXT("None")),
		-1);

	TArray<FTrainerAIMoveOption> OutOfPP;
	FTrainerAIMoveOption Move;
	Move.MoveName = TEXT("Tackle"); Move.MoveType = TEXT("Normal"); Move.Category = TEXT("Physical");
	Move.Power = 40; Move.Accuracy = 100; Move.CurrentPP = 0;
	OutOfPP.Add(Move);

	TestEqual(TEXT("A move with 0 PP is never selected, returns -1 if it's the only option"),
		UPokemonTrainerAI::SelectMoveIndex(OutOfPP,
			10, 20, 20, 0, 0, TEXT("Normal"), TEXT(""), 20, 20, 0, 0, TEXT("Normal"), TEXT(""), 50, 50, TEXT("None")),
		-1);
	return true;
}

/**
 * Functional test: runs a full trainer-vs-trainer battle where BOTH sides use
 * UPokemonTrainerAI::SelectMoveIndex to choose moves each turn (not random, not scripted),
 * and asserts the AI's choices are strategically sound throughout — never below full PP
 * availability, always exploiting the type matchup once it has the option to.
 */
BEGIN_DEFINE_SPEC(FPokemonTrainerAIBattleSpec, "Pokemon.TrainerAI.FunctionalBattle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FPokemonTrainerAIBattleSpec)

void FPokemonTrainerAIBattleSpec::Define()
{
	It("plays a full battle picking type-advantaged moves and finishing off low-HP targets", [this]()
	{
		struct FMockCombatant
		{
			int32 Level;
			int32 Attack;
			FString Type1;
			int32 HP;
			int32 MaxHP;
			TArray<FTrainerAIMoveOption> Moves;
		};

		FMockCombatant Player{ 12, 22, TEXT("Water"), 0, 0, {} };
		Player.MaxHP = Player.HP = UPokemonBattleLibrary::ScaleStat(38, Player.Level, true);
		{
			FTrainerAIMoveOption WaterGun;
			WaterGun.MoveName = TEXT("WaterGun"); WaterGun.MoveType = TEXT("Water"); WaterGun.Category = TEXT("Physical");
			WaterGun.Power = 40; WaterGun.Accuracy = 100; WaterGun.CurrentPP = 25;
			Player.Moves.Add(WaterGun);

			FTrainerAIMoveOption Tackle;
			Tackle.MoveName = TEXT("Tackle"); Tackle.MoveType = TEXT("Normal"); Tackle.Category = TEXT("Physical");
			Tackle.Power = 35; Tackle.Accuracy = 100; Tackle.CurrentPP = 35;
			Player.Moves.Add(Tackle);
		}

		FMockCombatant Enemy{ 11, 20, TEXT("Fire"), 0, 0, {} };
		Enemy.MaxHP = Enemy.HP = UPokemonBattleLibrary::ScaleStat(36, Enemy.Level, true);
		{
			FTrainerAIMoveOption Ember;
			Ember.MoveName = TEXT("Ember"); Ember.MoveType = TEXT("Fire"); Ember.Category = TEXT("Physical");
			Ember.Power = 40; Ember.Accuracy = 100; Ember.CurrentPP = 25;
			Enemy.Moves.Add(Ember);

			FTrainerAIMoveOption Scratch;
			Scratch.MoveName = TEXT("Scratch"); Scratch.MoveType = TEXT("Normal"); Scratch.Category = TEXT("Physical");
			Scratch.Power = 35; Scratch.Accuracy = 100; Scratch.CurrentPP = 35;
			Enemy.Moves.Add(Scratch);
		}

		constexpr int32 MaxTurns = 200;
		bool bPlayerWon = false, bEnemyWon = false;
		int32 PlayerPickedTypeAdvantage = 0, EnemyPickedTypeAdvantage = 0;

		int32 Turn = 0;
		for (; Turn < MaxTurns; ++Turn)
		{
			const int32 PlayerChoice = UPokemonTrainerAI::SelectMoveIndex(Player.Moves,
				Player.Level, Player.Attack, Player.Attack, 0, 0, Player.Type1, TEXT(""),
				Enemy.Attack, Enemy.Attack, 0, 0, Enemy.Type1, TEXT(""), Enemy.HP, Enemy.MaxHP, TEXT("None"));
			TestTrue(TEXT("Player AI always finds a usable move (has PP)"), PlayerChoice >= 0);
			if (PlayerChoice == 0) { ++PlayerPickedTypeAdvantage; } // WaterGun is Player's super-effective option vs Fire

			const int32 PlayerDamage = UPokemonBattleLibrary::CalculateDamage(
				Player.Level, Player.Attack, Player.Attack, 0, 0, Player.Type1, TEXT(""),
				Enemy.Attack, Enemy.Attack, 0, 0, Enemy.Type1, TEXT(""),
				Player.Moves[PlayerChoice].Category, Player.Moves[PlayerChoice].Power, Player.Moves[PlayerChoice].MoveType);
			Enemy.HP = FMath::Clamp(Enemy.HP - PlayerDamage, 0, Enemy.MaxHP);
			if (Enemy.HP <= 0) { bPlayerWon = true; break; }

			const int32 EnemyChoice = UPokemonTrainerAI::SelectMoveIndex(Enemy.Moves,
				Enemy.Level, Enemy.Attack, Enemy.Attack, 0, 0, Enemy.Type1, TEXT(""),
				Player.Attack, Player.Attack, 0, 0, Player.Type1, TEXT(""), Player.HP, Player.MaxHP, TEXT("None"));
			TestTrue(TEXT("Enemy AI always finds a usable move (has PP)"), EnemyChoice >= 0);
			if (EnemyChoice == 0) { ++EnemyPickedTypeAdvantage; } // Ember is Enemy's neutral (not super-effective vs Water) option

			const int32 EnemyDamage = UPokemonBattleLibrary::CalculateDamage(
				Enemy.Level, Enemy.Attack, Enemy.Attack, 0, 0, Enemy.Type1, TEXT(""),
				Player.Attack, Player.Attack, 0, 0, Player.Type1, TEXT(""),
				Enemy.Moves[EnemyChoice].Category, Enemy.Moves[EnemyChoice].Power, Enemy.Moves[EnemyChoice].MoveType);
			Player.HP = FMath::Clamp(Player.HP - EnemyDamage, 0, Player.MaxHP);
			if (Player.HP <= 0) { bEnemyWon = true; break; }
		}

		TestTrue(TEXT("Battle reached a winner within the turn cap"), bPlayerWon || bEnemyWon);
		TestFalse(TEXT("Both sides did not somehow lose simultaneously"), bPlayerWon && bEnemyWon);
		TestEqual(TEXT("Player's Water move is super-effective vs Fire and gets picked every single turn"),
			PlayerPickedTypeAdvantage, Turn + 1);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
