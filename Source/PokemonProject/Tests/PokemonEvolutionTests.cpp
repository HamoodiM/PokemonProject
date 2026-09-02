#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../PokemonEvolution.h"
#include "../PokemonBattleLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonEvolutionHasRuleTest, "Pokemon.Evolution.HasEvolutionRule",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonEvolutionHasRuleTest::RunTest(const FString& Parameters)
{
	FEvolutionRule NoEvolution;
	TestFalse(TEXT("Empty EvolvesToSpeciesRow means no evolution"), UPokemonEvolutionLibrary::HasEvolutionRule(NoEvolution));

	FEvolutionRule ExplicitNone;
	ExplicitNone.EvolvesToSpeciesRow = TEXT("None");
	TestFalse(TEXT("\"None\" EvolvesToSpeciesRow means no evolution"), UPokemonEvolutionLibrary::HasEvolutionRule(ExplicitNone));

	FEvolutionRule Ivysaur;
	Ivysaur.EvolvesToSpeciesRow = TEXT("Ivysaur");
	Ivysaur.MinLevel = 16;
	TestTrue(TEXT("A real target species has an evolution rule"), UPokemonEvolutionLibrary::HasEvolutionRule(Ivysaur));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonEvolutionLevelTest, "Pokemon.Evolution.LevelBased",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonEvolutionLevelTest::RunTest(const FString& Parameters)
{
	FEvolutionRule Ivysaur;
	Ivysaur.EvolvesToSpeciesRow = TEXT("Ivysaur");
	Ivysaur.MinLevel = 16;

	FString EvolvedRow;
	TestTrue(TEXT("Crossing the threshold exactly (15->16) triggers evolution"),
		UPokemonEvolutionLibrary::TryLevelEvolution(Ivysaur, 15, 16, EvolvedRow));
	TestEqual(TEXT("Evolves into the rule's target species"), EvolvedRow, FString(TEXT("Ivysaur")));

	TestTrue(TEXT("A multi-level jump that crosses the threshold also triggers"),
		UPokemonEvolutionLibrary::TryLevelEvolution(Ivysaur, 10, 20, EvolvedRow));

	TestFalse(TEXT("Staying below the threshold does not evolve"),
		UPokemonEvolutionLibrary::TryLevelEvolution(Ivysaur, 14, 15, EvolvedRow));

	TestFalse(TEXT("Already at/above the threshold before this level-up does not re-trigger"),
		UPokemonEvolutionLibrary::TryLevelEvolution(Ivysaur, 16, 17, EvolvedRow));

	FEvolutionRule ItemOnly;
	ItemOnly.EvolvesToSpeciesRow = TEXT("Vulpix2");
	ItemOnly.RequiredItem = TEXT("FireStone");
	TestFalse(TEXT("A rule with no MinLevel never triggers a level evolution"),
		UPokemonEvolutionLibrary::TryLevelEvolution(ItemOnly, 10, 100, EvolvedRow));

	FEvolutionRule NoEvolution;
	TestFalse(TEXT("A species with no evolution never triggers"),
		UPokemonEvolutionLibrary::TryLevelEvolution(NoEvolution, 1, 100, EvolvedRow));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonEvolutionItemTest, "Pokemon.Evolution.ItemBased",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonEvolutionItemTest::RunTest(const FString& Parameters)
{
	FEvolutionRule FireStoneVulpix;
	FireStoneVulpix.EvolvesToSpeciesRow = TEXT("Ninetales");
	FireStoneVulpix.RequiredItem = TEXT("FireStone");

	FString EvolvedRow;
	TestTrue(TEXT("Using the matching stone triggers evolution"),
		UPokemonEvolutionLibrary::TryItemEvolution(FireStoneVulpix, TEXT("FireStone"), EvolvedRow));
	TestEqual(TEXT("Evolves into Ninetales"), EvolvedRow, FString(TEXT("Ninetales")));

	TestFalse(TEXT("A different item does not trigger evolution"),
		UPokemonEvolutionLibrary::TryItemEvolution(FireStoneVulpix, TEXT("WaterStone"), EvolvedRow));

	FEvolutionRule LevelOnly;
	LevelOnly.EvolvesToSpeciesRow = TEXT("Ivysaur");
	LevelOnly.MinLevel = 16;
	TestFalse(TEXT("A rule with no RequiredItem never triggers an item evolution"),
		UPokemonEvolutionLibrary::TryItemEvolution(LevelOnly, TEXT("FireStone"), EvolvedRow));

	FEvolutionRule NoEvolution;
	TestFalse(TEXT("A species with no evolution never triggers"),
		UPokemonEvolutionLibrary::TryItemEvolution(NoEvolution, TEXT("FireStone"), EvolvedRow));
	return true;
}

/**
 * Functional test: drives a Pokemon through several level-ups via the existing
 * UPokemonBattleLibrary::ComputeLevelUp EXP system and confirms evolution fires on exactly the
 * level-up that crosses the species' threshold, not before and not on later ones.
 */
BEGIN_DEFINE_SPEC(FPokemonEvolutionLevelUpSpec, "Pokemon.Evolution.FunctionalLevelUp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FPokemonEvolutionLevelUpSpec)

void FPokemonEvolutionLevelUpSpec::Define()
{
	It("evolves a leveling Pokemon on exactly the level-up that crosses its threshold", [this]()
	{
		FEvolutionRule Ivysaur;
		Ivysaur.EvolvesToSpeciesRow = TEXT("Ivysaur");
		Ivysaur.MinLevel = 16;

		int32 CurrentLevel = 10;
		int32 CurrentEXP = 1000; // 10^3, exactly at level 10's threshold
		int32 EvolutionTriggerCount = 0;
		FString EvolvedInto;

		// Award EXP in three chunks; the level-16 threshold should be crossed on the middle award
		// (10->11, then 11->16 crossing the threshold, then 16->18 with no re-trigger).
		const int32 ExpAwards[] = { 700, 3000, 2000 };
		for (const int32 Award : ExpAwards)
		{
			int32 NewLevel, NewEXP; bool bLeveledUp;
			UPokemonBattleLibrary::ComputeLevelUp(CurrentLevel, 100, CurrentEXP, Award, NewLevel, NewEXP, bLeveledUp);

			FString ThisEvolution;
			if (UPokemonEvolutionLibrary::TryLevelEvolution(Ivysaur, CurrentLevel, NewLevel, ThisEvolution))
			{
				++EvolutionTriggerCount;
				EvolvedInto = ThisEvolution;
			}

			CurrentLevel = NewLevel;
			CurrentEXP = NewEXP;
		}

		TestEqual(TEXT("Evolution triggers exactly once across the whole sequence"), EvolutionTriggerCount, 1);
		TestEqual(TEXT("Evolved into the expected species"), EvolvedInto, FString(TEXT("Ivysaur")));
		TestTrue(TEXT("Final level is at or above the evolution threshold"), CurrentLevel >= 16);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
