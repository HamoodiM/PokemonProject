#include "PokemonPokedex.h"

namespace PokemonPokedexInternal
{
	FPokedexEntry* FindOrAddEntry(TArray<FPokedexEntry>& Pokedex, const FString& SpeciesRow)
	{
		for (FPokedexEntry& Entry : Pokedex)
		{
			if (Entry.SpeciesRow == SpeciesRow)
			{
				return &Entry;
			}
		}

		FPokedexEntry& NewEntry = Pokedex.AddDefaulted_GetRef();
		NewEntry.SpeciesRow = SpeciesRow;
		return &NewEntry;
	}
}

bool UPokemonPokedexLibrary::MarkSpeciesSeen(TArray<FPokedexEntry>& Pokedex, const FString& SpeciesRow)
{
	FPokedexEntry* Entry = PokemonPokedexInternal::FindOrAddEntry(Pokedex, SpeciesRow);
	if (Entry->bSeen)
	{
		return false;
	}

	Entry->bSeen = true;
	return true;
}

bool UPokemonPokedexLibrary::MarkSpeciesCaught(TArray<FPokedexEntry>& Pokedex, const FString& SpeciesRow)
{
	FPokedexEntry* Entry = PokemonPokedexInternal::FindOrAddEntry(Pokedex, SpeciesRow);
	const bool bFirstCatch = !Entry->bCaught;

	Entry->bSeen = true;
	Entry->bCaught = true;
	++Entry->CaughtCount;

	return bFirstCatch;
}

bool UPokemonPokedexLibrary::FindEntry(const TArray<FPokedexEntry>& Pokedex, const FString& SpeciesRow, FPokedexEntry& OutEntry)
{
	for (const FPokedexEntry& Entry : Pokedex)
	{
		if (Entry.SpeciesRow == SpeciesRow)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}

float UPokemonPokedexLibrary::GetCompletionPercent(const TArray<FPokedexEntry>& Pokedex, int32 TotalSpeciesCount)
{
	if (TotalSpeciesCount <= 0)
	{
		return 0.0f;
	}

	int32 CaughtSpeciesCount = 0;
	for (const FPokedexEntry& Entry : Pokedex)
	{
		if (Entry.bCaught)
		{
			++CaughtSpeciesCount;
		}
	}

	return FMath::Clamp((static_cast<float>(CaughtSpeciesCount) / TotalSpeciesCount) * 100.0f, 0.0f, 100.0f);
}
