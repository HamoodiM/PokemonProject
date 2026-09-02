#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../PokemonHeldItems.h"
#include "../PokemonBattleLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonHeldItemFlatStatBonusTest, "Pokemon.HeldItem.FlatStatBonus",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonHeldItemFlatStatBonusTest::RunTest(const FString& Parameters)
{
	FHeldItemEffect AssaultVest;
	AssaultVest.EffectType = EHeldItemEffectType::FlatStatBonus;
	AssaultVest.TargetStat = TEXT("SpDefence");
	AssaultVest.FlatStatBonus = 50;

	int32 NewStat;
	TestTrue(TEXT("Assault Vest applies to matching stat"), UPokemonHeldItemLibrary::ApplyFlatStatBonus(AssaultVest, TEXT("SpDefence"), 60, NewStat));
	TestEqual(TEXT("SpDefence boosted by the flat bonus"), NewStat, 110);

	TestFalse(TEXT("Assault Vest does not apply to a different stat"), UPokemonHeldItemLibrary::ApplyFlatStatBonus(AssaultVest, TEXT("Defence"), 60, NewStat));
	TestEqual(TEXT("Non-matching stat is unchanged"), NewStat, 60);

	FHeldItemEffect NoEffect;
	TestFalse(TEXT("An item with no effect never applies a bonus"), UPokemonHeldItemLibrary::ApplyFlatStatBonus(NoEffect, TEXT("SpDefence"), 60, NewStat));
	TestEqual(TEXT("Unchanged"), NewStat, 60);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonHeldItemDamageBoostTest, "Pokemon.HeldItem.DamageBoostWithRecoil",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonHeldItemDamageBoostTest::RunTest(const FString& Parameters)
{
	FHeldItemEffect LifeOrb;
	LifeOrb.EffectType = EHeldItemEffectType::DamageBoostWithRecoil;
	LifeOrb.DamageMultiplier = 1.3f;
	LifeOrb.RecoilFraction = 0.1f;

	TestEqual(TEXT("Life Orb boosts damage by its multiplier"), UPokemonHeldItemLibrary::GetDamageMultiplier(LifeOrb), 1.3f);
	TestEqual(TEXT("Life Orb recoil is 10% of damage dealt, rounded up"), UPokemonHeldItemLibrary::CalculateHeldItemRecoil(LifeOrb, 25), 3);
	TestEqual(TEXT("Life Orb recoil floors at 1 for tiny damage"), UPokemonHeldItemLibrary::CalculateHeldItemRecoil(LifeOrb, 1), 1);
	TestEqual(TEXT("No recoil if no damage was dealt"), UPokemonHeldItemLibrary::CalculateHeldItemRecoil(LifeOrb, 0), 0);

	FHeldItemEffect NoEffect;
	TestEqual(TEXT("An item with no effect never boosts damage"), UPokemonHeldItemLibrary::GetDamageMultiplier(NoEffect), 1.0f);
	TestEqual(TEXT("An item with no effect never deals recoil"), UPokemonHeldItemLibrary::CalculateHeldItemRecoil(NoEffect, 100), 0);
	return true;
}

/**
 * Functional test: an Assault Vest-holding defender takes less special damage than an unequipped
 * one, and a Life Orb-holding attacker deals more damage but takes recoil for it — both resolved
 * through the real UPokemonBattleLibrary::CalculateDamage formula, not a mock.
 */
BEGIN_DEFINE_SPEC(FPokemonHeldItemBattleSpec, "Pokemon.HeldItem.FunctionalBattle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FPokemonHeldItemBattleSpec)

void FPokemonHeldItemBattleSpec::Define()
{
	It("changes real battle damage output when items are equipped", [this]()
	{
		constexpr int32 AttackerLevel = 15, AttackerSpAttack = 30, BaseSpDefence = 25;

		const int32 DamageNoItem = UPokemonBattleLibrary::CalculateDamage(
			AttackerLevel, 0, AttackerSpAttack, 0, 0, TEXT("Water"), TEXT(""),
			BaseSpDefence, BaseSpDefence, 0, 0, TEXT("Fire"), TEXT(""),
			TEXT("Special"), 50, TEXT("Water"));

		FHeldItemEffect AssaultVest;
		AssaultVest.EffectType = EHeldItemEffectType::FlatStatBonus;
		AssaultVest.TargetStat = TEXT("SpDefence");
		AssaultVest.FlatStatBonus = 50;

		int32 BoostedSpDefence;
		UPokemonHeldItemLibrary::ApplyFlatStatBonus(AssaultVest, TEXT("SpDefence"), BaseSpDefence, BoostedSpDefence);

		const int32 DamageWithAssaultVest = UPokemonBattleLibrary::CalculateDamage(
			AttackerLevel, 0, AttackerSpAttack, 0, 0, TEXT("Water"), TEXT(""),
			BoostedSpDefence, BoostedSpDefence, 0, 0, TEXT("Fire"), TEXT(""),
			TEXT("Special"), 50, TEXT("Water"));

		TestTrue(TEXT("Assault Vest meaningfully reduces incoming special damage"), DamageWithAssaultVest < DamageNoItem);

		// Life Orb: the attacker deals more damage but pays recoil for it.
		FHeldItemEffect LifeOrb;
		LifeOrb.EffectType = EHeldItemEffectType::DamageBoostWithRecoil;
		LifeOrb.DamageMultiplier = 1.3f;
		LifeOrb.RecoilFraction = 0.1f;

		const int32 BoostedDamage = FMath::FloorToInt(DamageNoItem * UPokemonHeldItemLibrary::GetDamageMultiplier(LifeOrb));
		const int32 Recoil = UPokemonHeldItemLibrary::CalculateHeldItemRecoil(LifeOrb, BoostedDamage);

		TestTrue(TEXT("Life Orb increases outgoing damage"), BoostedDamage > DamageNoItem);
		TestTrue(TEXT("Life Orb recoil is nonzero when damage was dealt"), Recoil > 0);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
