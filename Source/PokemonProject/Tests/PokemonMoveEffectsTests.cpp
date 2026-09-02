#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../PokemonMoveEffects.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonMoveEffectStatChangeTest, "Pokemon.MoveEffect.StatChange",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonMoveEffectStatChangeTest::RunTest(const FString& Parameters)
{
	FMoveEffect SwordsDance;
	SwordsDance.EffectType = EMoveEffectType::StatChange;
	SwordsDance.TargetStat = TEXT("Attack");
	SwordsDance.StatStageDelta = 2;
	SwordsDance.bAppliesToSelf = true;

	int32 NewStage;
	TestTrue(TEXT("StatChange effect reports it applied"), UPokemonMoveEffectLibrary::ResolveStatChange(SwordsDance, 0, NewStage));
	TestEqual(TEXT("Swords Dance raises Attack by 2 stages"), NewStage, 2);

	// Clamping still applies at the cap.
	TestTrue(TEXT("StatChange still applies near the cap"), UPokemonMoveEffectLibrary::ResolveStatChange(SwordsDance, 5, NewStage));
	TestEqual(TEXT("Stat stage clamps at +6"), NewStage, 6);

	FMoveEffect Withdraw;
	Withdraw.EffectType = EMoveEffectType::StatChange;
	Withdraw.TargetStat = TEXT("Defence");
	Withdraw.StatStageDelta = 1;
	TestTrue(TEXT("Withdraw raises Defence by 1"), UPokemonMoveEffectLibrary::ResolveStatChange(Withdraw, 0, NewStage));
	TestEqual(TEXT("Withdraw result"), NewStage, 1);

	// A damage-only move (no stat effect) is a no-op.
	FMoveEffect NoEffect;
	TestFalse(TEXT("None-type effect does not apply a stat change"), UPokemonMoveEffectLibrary::ResolveStatChange(NoEffect, 3, NewStage));
	TestEqual(TEXT("No-op leaves the stage unchanged"), NewStage, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonMoveEffectStatusApplicationTest, "Pokemon.MoveEffect.StatusApplication",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonMoveEffectStatusApplicationTest::RunTest(const FString& Parameters)
{
	FMoveEffect ThunderWave;
	ThunderWave.EffectType = EMoveEffectType::ApplyStatus;
	ThunderWave.StatusToApply = TEXT("Paralysis");
	ThunderWave.StatusChance = 100; // guaranteed for a deterministic test

	FString NewStatus;
	int32 NewCounter;
	TestTrue(TEXT("Guaranteed-chance status move applies"), UPokemonMoveEffectLibrary::ResolveStatusApplication(ThunderWave, TEXT("None"), NewStatus, NewCounter));
	TestEqual(TEXT("Target is now Paralyzed"), NewStatus, FString(TEXT("Paralysis")));
	TestEqual(TEXT("Paralysis has no duration counter"), NewCounter, 0);

	// Can't stack a second status on an already-afflicted target (matches UPokemonBattleLibrary::ShouldApplyStatus).
	FString UnchangedStatus;
	TestFalse(TEXT("Cannot apply a status over an existing one"), UPokemonMoveEffectLibrary::ResolveStatusApplication(ThunderWave, TEXT("Burn"), UnchangedStatus, NewCounter));
	TestEqual(TEXT("Status stays as the pre-existing one"), UnchangedStatus, FString(TEXT("Burn")));

	// Applying Sleep rolls a real 1-3 turn counter via UPokemonStatusEffects.
	FMoveEffect Spore;
	Spore.EffectType = EMoveEffectType::ApplyStatus;
	Spore.StatusToApply = TEXT("Sleep");
	Spore.StatusChance = 100;
	TestTrue(TEXT("Spore applies Sleep"), UPokemonMoveEffectLibrary::ResolveStatusApplication(Spore, TEXT("None"), NewStatus, NewCounter));
	TestEqual(TEXT("Applied status is Sleep"), NewStatus, FString(TEXT("Sleep")));
	TestTrue(TEXT("Sleep gets a real 1-3 turn counter"), NewCounter >= 1 && NewCounter <= 3);

	// Non-status effects are a no-op.
	FMoveEffect NoEffect;
	TestFalse(TEXT("None-type effect never applies a status"), UPokemonMoveEffectLibrary::ResolveStatusApplication(NoEffect, TEXT("None"), NewStatus, NewCounter));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonMoveEffectRecoilTest, "Pokemon.MoveEffect.Recoil",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonMoveEffectRecoilTest::RunTest(const FString& Parameters)
{
	FMoveEffect DoubleEdge;
	DoubleEdge.EffectType = EMoveEffectType::Recoil;
	DoubleEdge.RecoilFraction = 1.0f / 3.0f;

	TestEqual(TEXT("Double-Edge recoil is 1/3 of damage dealt, rounded up"), UPokemonMoveEffectLibrary::CalculateRecoilDamage(DoubleEdge, 30), 10);
	TestEqual(TEXT("Recoil floors at 1 even for tiny damage"), UPokemonMoveEffectLibrary::CalculateRecoilDamage(DoubleEdge, 1), 1);
	TestEqual(TEXT("No recoil if no damage was dealt"), UPokemonMoveEffectLibrary::CalculateRecoilDamage(DoubleEdge, 0), 0);

	FMoveEffect NoEffect;
	TestEqual(TEXT("Non-recoil effect never deals recoil"), UPokemonMoveEffectLibrary::CalculateRecoilDamage(NoEffect, 100), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonMoveEffectHealTest, "Pokemon.MoveEffect.Heal",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonMoveEffectHealTest::RunTest(const FString& Parameters)
{
	FMoveEffect Recover;
	Recover.EffectType = EMoveEffectType::Heal;
	Recover.HealFraction = 1.0f;

	TestEqual(TEXT("Recover heals the full MaxHP"), UPokemonMoveEffectLibrary::CalculateHealAmount(Recover, 80), 80);

	FMoveEffect SoftBoiled;
	SoftBoiled.EffectType = EMoveEffectType::Heal;
	SoftBoiled.HealFraction = 0.5f;
	TestEqual(TEXT("Half-heal moves restore half MaxHP"), UPokemonMoveEffectLibrary::CalculateHealAmount(SoftBoiled, 80), 40);
	TestEqual(TEXT("Heal amount floors at 1"), UPokemonMoveEffectLibrary::CalculateHealAmount(SoftBoiled, 1), 1);

	FMoveEffect NoEffect;
	TestEqual(TEXT("Non-heal effect never heals"), UPokemonMoveEffectLibrary::CalculateHealAmount(NoEffect, 80), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonMoveEffectTwoTurnTest, "Pokemon.MoveEffect.TwoTurnCharge",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonMoveEffectTwoTurnTest::RunTest(const FString& Parameters)
{
	bool bShouldExecute, bNewCharging;

	UPokemonMoveEffectLibrary::ProcessTwoTurnMove(false, bShouldExecute, bNewCharging);
	TestFalse(TEXT("First use of Solar Beam does not deal damage yet"), bShouldExecute);
	TestTrue(TEXT("First use starts charging"), bNewCharging);

	UPokemonMoveEffectLibrary::ProcessTwoTurnMove(true, bShouldExecute, bNewCharging);
	TestTrue(TEXT("Second use (already charging) releases the attack"), bShouldExecute);
	TestFalse(TEXT("Charging state clears after release"), bNewCharging);
	return true;
}

/**
 * Functional test: plays out a short scripted turn sequence exercising every move-effect category
 * in one battle, mirroring how ResolveMove would chain them turn-to-turn.
 */
BEGIN_DEFINE_SPEC(FPokemonMoveEffectBattleSpec, "Pokemon.MoveEffect.FunctionalBattle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FPokemonMoveEffectBattleSpec)

void FPokemonMoveEffectBattleSpec::Define()
{
	It("chains stat change, status application, recoil, heal, and a two-turn charge move correctly", [this]()
	{
		int32 AttackStage = 0;
		FString TargetStatus = TEXT("None");
		int32 TargetStatusCounter = 0;
		int32 AttackerHP = 50, AttackerMaxHP = 50;
		bool bCharging = false;

		// Turn 1: Swords Dance.
		FMoveEffect SwordsDance;
		SwordsDance.EffectType = EMoveEffectType::StatChange;
		SwordsDance.StatStageDelta = 2;
		int32 NewStage;
		TestTrue(TEXT("Swords Dance applies"), UPokemonMoveEffectLibrary::ResolveStatChange(SwordsDance, AttackStage, NewStage));
		AttackStage = NewStage;
		TestEqual(TEXT("Attack is now +2"), AttackStage, 2);

		// Turn 2: Thunder Wave on the opponent.
		FMoveEffect ThunderWave;
		ThunderWave.EffectType = EMoveEffectType::ApplyStatus;
		ThunderWave.StatusToApply = TEXT("Paralysis");
		ThunderWave.StatusChance = 100;
		FString NewStatus; int32 NewCounter;
		TestTrue(TEXT("Thunder Wave applies Paralysis"), UPokemonMoveEffectLibrary::ResolveStatusApplication(ThunderWave, TargetStatus, NewStatus, NewCounter));
		TargetStatus = NewStatus; TargetStatusCounter = NewCounter;
		TestEqual(TEXT("Opponent is Paralyzed"), TargetStatus, FString(TEXT("Paralysis")));

		// Turn 3: Double-Edge deals 30 damage, attacker recoils.
		FMoveEffect DoubleEdge;
		DoubleEdge.EffectType = EMoveEffectType::Recoil;
		DoubleEdge.RecoilFraction = 1.0f / 3.0f;
		constexpr int32 DamageDealt = 30;
		const int32 Recoil = UPokemonMoveEffectLibrary::CalculateRecoilDamage(DoubleEdge, DamageDealt);
		AttackerHP = FMath::Clamp(AttackerHP - Recoil, 0, AttackerMaxHP);
		TestEqual(TEXT("Recoil damage taken"), Recoil, 10);
		TestEqual(TEXT("Attacker HP after recoil"), AttackerHP, 40);

		// Turn 4: Recover heals back to full.
		FMoveEffect Recover;
		Recover.EffectType = EMoveEffectType::Heal;
		Recover.HealFraction = 1.0f;
		const int32 HealAmount = UPokemonMoveEffectLibrary::CalculateHealAmount(Recover, AttackerMaxHP);
		AttackerHP = FMath::Clamp(AttackerHP + HealAmount, 0, AttackerMaxHP);
		TestEqual(TEXT("Attacker HP restored to full"), AttackerHP, AttackerMaxHP);

		// Turns 5-6: Solar Beam charges then fires.
		bool bShouldExecute, bNewCharging;
		UPokemonMoveEffectLibrary::ProcessTwoTurnMove(bCharging, bShouldExecute, bNewCharging);
		bCharging = bNewCharging;
		TestFalse(TEXT("Solar Beam turn 1 doesn't deal damage"), bShouldExecute);
		TestTrue(TEXT("Solar Beam turn 1 starts charging"), bCharging);

		UPokemonMoveEffectLibrary::ProcessTwoTurnMove(bCharging, bShouldExecute, bNewCharging);
		bCharging = bNewCharging;
		TestTrue(TEXT("Solar Beam turn 2 releases the attack"), bShouldExecute);
		TestFalse(TEXT("Solar Beam is done charging after release"), bCharging);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
