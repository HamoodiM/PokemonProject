#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../PokemonPokedex.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonPokedexSeenTest, "Pokemon.Pokedex.MarkSpeciesSeen",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonPokedexSeenTest::RunTest(const FString& Parameters)
{
	TArray<FPokedexEntry> Pokedex;

	TestTrue(TEXT("First sighting of a species returns true"), UPokemonPokedexLibrary::MarkSpeciesSeen(Pokedex, TEXT("Squirtle")));
	TestEqual(TEXT("Exactly one entry was created"), Pokedex.Num(), 1);
	TestTrue(TEXT("New entry is marked seen"), Pokedex[0].bSeen);
	TestFalse(TEXT("New entry is not marked caught"), Pokedex[0].bCaught);

	TestFalse(TEXT("Re-sighting an already-seen species returns false"), UPokemonPokedexLibrary::MarkSpeciesSeen(Pokedex, TEXT("Squirtle")));
	TestEqual(TEXT("No duplicate entry was created"), Pokedex.Num(), 1);

	UPokemonPokedexLibrary::MarkSpeciesSeen(Pokedex, TEXT("Charmander"));
	TestEqual(TEXT("A second distinct species adds a second entry"), Pokedex.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonPokedexCaughtTest, "Pokemon.Pokedex.MarkSpeciesCaught",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonPokedexCaughtTest::RunTest(const FString& Parameters)
{
	TArray<FPokedexEntry> Pokedex;

	TestTrue(TEXT("First catch of a species (no prior sighting) returns true"), UPokemonPokedexLibrary::MarkSpeciesCaught(Pokedex, TEXT("Squirtle")));
	TestEqual(TEXT("Catching creates exactly one entry"), Pokedex.Num(), 1);
	TestTrue(TEXT("Catching also marks the species seen"), Pokedex[0].bSeen);
	TestTrue(TEXT("Entry is marked caught"), Pokedex[0].bCaught);
	TestEqual(TEXT("CaughtCount starts at 1"), Pokedex[0].CaughtCount, 1);

	TestFalse(TEXT("Catching an already-caught species returns false (not a first catch)"), UPokemonPokedexLibrary::MarkSpeciesCaught(Pokedex, TEXT("Squirtle")));
	TestEqual(TEXT("CaughtCount still increments on repeat catches"), Pokedex[0].CaughtCount, 2);
	TestEqual(TEXT("Still exactly one entry, not a duplicate"), Pokedex.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPokemonPokedexFindAndCompletionTest, "Pokemon.Pokedex.FindEntryAndCompletion",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FPokemonPokedexFindAndCompletionTest::RunTest(const FString& Parameters)
{
	TArray<FPokedexEntry> Pokedex;
	UPokemonPokedexLibrary::MarkSpeciesSeen(Pokedex, TEXT("Bulbasaur"));

	FPokedexEntry FoundEntry;
	TestTrue(TEXT("FindEntry finds an existing species"), UPokemonPokedexLibrary::FindEntry(Pokedex, TEXT("Bulbasaur"), FoundEntry));
	TestEqual(TEXT("Found entry has the right species row"), FoundEntry.SpeciesRow, FString(TEXT("Bulbasaur")));

	FPokedexEntry NotFound;
	TestFalse(TEXT("FindEntry returns false for a species never encountered"), UPokemonPokedexLibrary::FindEntry(Pokedex, TEXT("Mewtwo"), NotFound));

	TestEqual(TEXT("Zero total species count never divides by zero"), UPokemonPokedexLibrary::GetCompletionPercent(Pokedex, 0), 0.0f);
	TestEqual(TEXT("Seen-but-not-caught species don't count toward completion"), UPokemonPokedexLibrary::GetCompletionPercent(Pokedex, 3), 0.0f);

	UPokemonPokedexLibrary::MarkSpeciesCaught(Pokedex, TEXT("Bulbasaur"));
	TestEqual(TEXT("One caught out of three total is 33.33%"), UPokemonPokedexLibrary::GetCompletionPercent(Pokedex, 3), 100.0f / 3.0f);
	return true;
}

/**
 * Functional test: a short play sequence — encounter three species (seen), catch two of them
 * (one caught twice) — mirroring what a real battle/capture flow would call, then checks the
 * resulting Pokedex state and completion percentage all together.
 */
BEGIN_DEFINE_SPEC(FPokemonPokedexPlaySequenceSpec, "Pokemon.Pokedex.FunctionalPlaySequence",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FPokemonPokedexPlaySequenceSpec)

void FPokemonPokedexPlaySequenceSpec::Define()
{
	It("tracks seen/caught state correctly across a play sequence and computes completion", [this]()
	{
		TArray<FPokedexEntry> Pokedex;
		constexpr int32 TotalSpeciesInGame = 3;

		// Encounter all three species in the wild.
		UPokemonPokedexLibrary::MarkSpeciesSeen(Pokedex, TEXT("Squirtle"));
		UPokemonPokedexLibrary::MarkSpeciesSeen(Pokedex, TEXT("Charmander"));
		UPokemonPokedexLibrary::MarkSpeciesSeen(Pokedex, TEXT("Bulbasaur"));
		TestEqual(TEXT("All three species are recorded as seen"), Pokedex.Num(), 3);

		// Catch Squirtle twice, Charmander once, never catch Bulbasaur.
		UPokemonPokedexLibrary::MarkSpeciesCaught(Pokedex, TEXT("Squirtle"));
		UPokemonPokedexLibrary::MarkSpeciesCaught(Pokedex, TEXT("Squirtle"));
		UPokemonPokedexLibrary::MarkSpeciesCaught(Pokedex, TEXT("Charmander"));

		FPokedexEntry Squirtle, Charmander, Bulbasaur;
		UPokemonPokedexLibrary::FindEntry(Pokedex, TEXT("Squirtle"), Squirtle);
		UPokemonPokedexLibrary::FindEntry(Pokedex, TEXT("Charmander"), Charmander);
		UPokemonPokedexLibrary::FindEntry(Pokedex, TEXT("Bulbasaur"), Bulbasaur);

		TestEqual(TEXT("Squirtle was caught twice"), Squirtle.CaughtCount, 2);
		TestEqual(TEXT("Charmander was caught once"), Charmander.CaughtCount, 1);
		TestFalse(TEXT("Bulbasaur was never caught"), Bulbasaur.bCaught);
		TestTrue(TEXT("Bulbasaur was still seen"), Bulbasaur.bSeen);

		TestEqual(TEXT("2 of 3 species caught is 66.67% complete"),
			UPokemonPokedexLibrary::GetCompletionPercent(Pokedex, TotalSpeciesInGame), 200.0f / 3.0f);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
