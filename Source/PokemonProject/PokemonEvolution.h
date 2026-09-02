#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PokemonEvolution.generated.h"

/**
 * One evolution edge for a species: either level-based, item-based, or (per the data) both fields
 * populated at once — callers just check whichever trigger they're resolving. A species with no
 * evolution has EvolvesToSpeciesRow empty/"None". Only a single next-stage edge per species is
 * modeled (Bulbasaur -> Ivysaur, not a multi-branch tree) — branching evolution (Eevee-style)
 * would need an array of these instead, out of scope this session.
 */
USTRUCT(BlueprintType)
struct FEvolutionRule
{
	GENERATED_BODY()

	/** DT_PokemonSpecies row name to evolve into. Empty/"None" means this species doesn't evolve. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|Evolution")
	FString EvolvesToSpeciesRow;

	/** 0 = not level-triggered. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|Evolution")
	int32 MinLevel = 0;

	/** Empty/"None" = not item-triggered. DT_Items row name. */
	UPROPERTY(BlueprintReadWrite, Category = "Pokemon|Evolution")
	FString RequiredItem;
};

/** Pure C++ evolution-trigger resolution. Doesn't touch DataTables directly — caller looks up the FEvolutionRule and passes it in, same broken-out-primitives convention as the rest of this module. */
UCLASS()
class POKEMONPROJECT_API UPokemonEvolutionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** True if Rule targets a real species (has something to evolve into at all). */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Evolution")
	static bool HasEvolutionRule(const FEvolutionRule& Rule);

	/**
	 * True only on the level-up that crosses the threshold (OldLevel below MinLevel, NewLevel at
	 * or above it) — call once per level-up with the before/after level, not every turn.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Evolution")
	static bool TryLevelEvolution(const FEvolutionRule& Rule, int32 OldLevel, int32 NewLevel, FString& OutEvolvedSpeciesRow);

	/** True if Rule is item-triggered and UsedItemRow matches RequiredItem exactly. */
	UFUNCTION(BlueprintCallable, Category = "Pokemon|Evolution")
	static bool TryItemEvolution(const FEvolutionRule& Rule, const FString& UsedItemRow, FString& OutEvolvedSpeciesRow);
};
