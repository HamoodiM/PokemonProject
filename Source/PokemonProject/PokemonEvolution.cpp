#include "PokemonEvolution.h"

bool UPokemonEvolutionLibrary::HasEvolutionRule(const FEvolutionRule& Rule)
{
	return !Rule.EvolvesToSpeciesRow.IsEmpty() && Rule.EvolvesToSpeciesRow != TEXT("None");
}

bool UPokemonEvolutionLibrary::TryLevelEvolution(const FEvolutionRule& Rule, int32 OldLevel, int32 NewLevel, FString& OutEvolvedSpeciesRow)
{
	OutEvolvedSpeciesRow.Empty();

	if (!HasEvolutionRule(Rule) || Rule.MinLevel <= 0)
	{
		return false;
	}

	if (OldLevel < Rule.MinLevel && NewLevel >= Rule.MinLevel)
	{
		OutEvolvedSpeciesRow = Rule.EvolvesToSpeciesRow;
		return true;
	}

	return false;
}

bool UPokemonEvolutionLibrary::TryItemEvolution(const FEvolutionRule& Rule, const FString& UsedItemRow, FString& OutEvolvedSpeciesRow)
{
	OutEvolvedSpeciesRow.Empty();

	if (!HasEvolutionRule(Rule) || Rule.RequiredItem.IsEmpty() || Rule.RequiredItem == TEXT("None"))
	{
		return false;
	}

	if (UsedItemRow == Rule.RequiredItem)
	{
		OutEvolvedSpeciesRow = Rule.EvolvesToSpeciesRow;
		return true;
	}

	return false;
}
