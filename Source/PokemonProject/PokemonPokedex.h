#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PokemonPokedex.generated.h"

/** One species' Pokedex record. */
USTRUCT(BlueprintType)
struct FPokedexEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|Pokedex")
	FString SpeciesRow;

	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|Pokedex")
	bool bSeen = false;

	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|Pokedex")
	bool bCaught = false;

	/** Total number caught over the game, not just 0/1 — lets a completion screen show "x3" etc. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|Pokedex")
	int32 CaughtCount = 0;
};

/**
 * Pure C++ Pokedex state transitions, operating on a caller-owned TArray<FPokedexEntry> (the
 * array this project would persist via SG_PokemonSave, same PopulateX/GetXData split as Money/
 * Inventory — that Blueprint-side wiring isn't done this session, see HANDOFF.md).
 */
UCLASS()
class POKEMONPROJECT_API UPokemonPokedexLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Marks a species as seen, creating its entry if one doesn't exist yet. Returns true only if
	 * this call actually changed something (first sighting) — false if already seen.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Pokedex")
	static bool MarkSpeciesSeen(UPARAM(ref) TArray<FPokedexEntry>& Pokedex, const FString& SpeciesRow);

	/**
	 * Marks a species as caught (also marks it seen, creating the entry if needed) and increments
	 * CaughtCount every call. Returns true only if this was the species' first-ever catch.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Pokedex")
	static bool MarkSpeciesCaught(UPARAM(ref) TArray<FPokedexEntry>& Pokedex, const FString& SpeciesRow);

	UFUNCTION(BlueprintCallable, Category = "Pokemon|Pokedex")
	static bool FindEntry(const TArray<FPokedexEntry>& Pokedex, const FString& SpeciesRow, FPokedexEntry& OutEntry);

	/** (# species caught / TotalSpeciesCount) * 100, clamped to [0, 100]. 0 if TotalSpeciesCount <= 0. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Pokedex")
	static float GetCompletionPercent(const TArray<FPokedexEntry>& Pokedex, int32 TotalSpeciesCount);
};
