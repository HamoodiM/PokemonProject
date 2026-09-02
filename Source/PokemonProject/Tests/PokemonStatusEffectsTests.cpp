#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../PokemonStatusEffects.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonSleepStatusTest, "Pokemon.Status.Sleep",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonSleepStatusTest::RunTest(const FString& Parameters)
{
	for (int32 i = 0; i < 50; ++i)
	{
		const int32 Counter = UPokemonStatusEffects::RollInitialStatusCounter(TEXT("Sleep"));
		TestTrue(TEXT("Sleep counter rolls between 1 and 3 turns"), Counter >= 1 && Counter <= 3);
	}

	// Counter > 1: cannot act, counter decrements, status persists.
	int32 NewCounter; FString NewStatus; bool bCleared;
	bool bCanAct = UPokemonStatusEffects::ProcessTurnStartStatus(TEXT("Sleep"), 2, NewCounter, NewStatus, bCleared);
	TestFalse(TEXT("Asleep with turns remaining cannot act"), bCanAct);
	TestEqual(TEXT("Counter decrements by 1"), NewCounter, 1);
	TestEqual(TEXT("Status stays Sleep while counter > 0"), NewStatus, FString(TEXT("Sleep")));
	TestFalse(TEXT("Status not yet cleared"), bCleared);

	// Counter reaching 0 this turn: wakes up and can act immediately.
	bCanAct = UPokemonStatusEffects::ProcessTurnStartStatus(TEXT("Sleep"), 1, NewCounter, NewStatus, bCleared);
	TestTrue(TEXT("Wakes up and can act the turn the counter hits 0"), bCanAct);
	TestEqual(TEXT("Counter clamped at 0"), NewCounter, 0);
	TestEqual(TEXT("Status clears to None on wake"), NewStatus, FString(TEXT("None")));
	TestTrue(TEXT("bStatusJustCleared is set on wake"), bCleared);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonParalysisStatusTest, "Pokemon.Status.Paralysis",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonParalysisStatusTest::RunTest(const FString& Parameters)
{
	// Statistical check over many rolls: fully-paralyzed should happen sometimes but not always/never.
	int32 ActedCount = 0;
	constexpr int32 Trials = 400;
	for (int32 i = 0; i < Trials; ++i)
	{
		int32 NewCounter; FString NewStatus; bool bCleared;
		if (UPokemonStatusEffects::ProcessTurnStartStatus(TEXT("Paralysis"), 0, NewCounter, NewStatus, bCleared))
		{
			++ActedCount;
		}
		TestEqual(TEXT("Paralysis never touches the counter"), NewCounter, 0);
		TestEqual(TEXT("Paralysis never clears itself"), NewStatus, FString(TEXT("Paralysis")));
		TestFalse(TEXT("Paralysis is never reported as just-cleared"), bCleared);
	}
	TestTrue(TEXT("Paralysis sometimes allows acting"), ActedCount > 0);
	TestTrue(TEXT("Paralysis sometimes fully prevents acting"), ActedCount < Trials);

	TestEqual(TEXT("Paralysis cuts Speed to 1/4"), UPokemonStatusEffects::GetStatusSpeedMultiplier(TEXT("Paralysis")), 0.25f);
	TestEqual(TEXT("Paralysis applies an accuracy penalty"), UPokemonStatusEffects::GetStatusAccuracyMultiplier(TEXT("Paralysis")), 0.75f);
	TestEqual(TEXT("No status leaves speed unaffected"), UPokemonStatusEffects::GetStatusSpeedMultiplier(TEXT("None")), 1.0f);
	TestEqual(TEXT("No status leaves accuracy unaffected"), UPokemonStatusEffects::GetStatusAccuracyMultiplier(TEXT("None")), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonFreezeStatusTest, "Pokemon.Status.Freeze",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonFreezeStatusTest::RunTest(const FString& Parameters)
{
	int32 ThawCount = 0;
	constexpr int32 Trials = 400;
	for (int32 i = 0; i < Trials; ++i)
	{
		int32 NewCounter; FString NewStatus; bool bCleared;
		const bool bCanAct = UPokemonStatusEffects::ProcessTurnStartStatus(TEXT("Freeze"), 0, NewCounter, NewStatus, bCleared);
		if (bCanAct)
		{
			++ThawCount;
			TestEqual(TEXT("Thawing clears status to None"), NewStatus, FString(TEXT("None")));
			TestTrue(TEXT("Thawing sets bStatusJustCleared"), bCleared);
		}
		else
		{
			TestEqual(TEXT("Staying frozen keeps status as Freeze"), NewStatus, FString(TEXT("Freeze")));
		}
	}
	TestTrue(TEXT("Freeze sometimes thaws"), ThawCount > 0);
	TestTrue(TEXT("Freeze does not always thaw"), ThawCount < Trials);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonBurnStatusTest, "Pokemon.Status.Burn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonBurnStatusTest::RunTest(const FString& Parameters)
{
	int32 NewCounter; FString NewStatus; bool bCleared;
	const bool bCanAct = UPokemonStatusEffects::ProcessTurnStartStatus(TEXT("Burn"), 0, NewCounter, NewStatus, bCleared);
	TestTrue(TEXT("Burn never prevents acting"), bCanAct);
	TestFalse(TEXT("Burn never self-clears"), bCleared);

	TestEqual(TEXT("Burn halves physical Attack"), UPokemonStatusEffects::GetStatusAttackMultiplier(TEXT("Burn"), TEXT("Physical")), 0.5f);
	TestEqual(TEXT("Burn does not affect Special Attack"), UPokemonStatusEffects::GetStatusAttackMultiplier(TEXT("Burn"), TEXT("Special")), 1.0f);
	TestEqual(TEXT("No status leaves physical Attack unaffected"), UPokemonStatusEffects::GetStatusAttackMultiplier(TEXT("None"), TEXT("Physical")), 1.0f);

	TestEqual(TEXT("Burn chip damage is MaxHP/16, floored at 1"), UPokemonStatusEffects::GetStatusChipDamage(TEXT("Burn"), 16), 1);
	TestEqual(TEXT("Burn chip damage scales with MaxHP"), UPokemonStatusEffects::GetStatusChipDamage(TEXT("Burn"), 160), 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonPoisonStatusTest, "Pokemon.Status.Poison",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonPoisonStatusTest::RunTest(const FString& Parameters)
{
	int32 NewCounter; FString NewStatus; bool bCleared;
	const bool bCanAct = UPokemonStatusEffects::ProcessTurnStartStatus(TEXT("Poison"), 0, NewCounter, NewStatus, bCleared);
	TestTrue(TEXT("Poison never prevents acting"), bCanAct);

	TestEqual(TEXT("Poison chip damage is MaxHP/8, floored at 1"), UPokemonStatusEffects::GetStatusChipDamage(TEXT("Poison"), 8), 1);
	TestEqual(TEXT("Poison chip damage scales with MaxHP"), UPokemonStatusEffects::GetStatusChipDamage(TEXT("Poison"), 80), 10);
	TestEqual(TEXT("Low MaxHP still deals at least 1 chip damage"), UPokemonStatusEffects::GetStatusChipDamage(TEXT("Poison"), 3), 1);
	TestEqual(TEXT("No chip damage for conditions without one"), UPokemonStatusEffects::GetStatusChipDamage(TEXT("Paralysis"), 100), 0);
	TestEqual(TEXT("Zero MaxHP never deals chip damage"), UPokemonStatusEffects::GetStatusChipDamage(TEXT("Poison"), 0), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
