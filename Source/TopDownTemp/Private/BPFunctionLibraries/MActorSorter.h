// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MActorSorter.generated.h"

/** BP function library for sorting actors by certain criteria */
UCLASS()
class UMActorSorter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	* Sorts an array of actors by their distance to a given location.
	* 
	* @param Actors Array of actors to sort.
	* @param Location The location to measure distance from.
	* @param SortedActors Output array of sorted actors.
	*/
	UFUNCTION(BlueprintCallable, Category = "Utilities|Actor")
	static void SortActorsByDistance(const TArray<AActor*>& Actors, const FVector& Location, TArray<AActor*>& SortedActors);

	/**
	 * Finds the closest actor to a given location.
	 *
	 * @param Actors Array of actors to check.
	 * @param Location The location to measure distance from.
	 * @return The closest actor to the location, or nullptr if the array is empty.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utilities|Actor")
	static AActor* GetClosestActorToLocation(const TArray<AActor*>& Actors, const FVector& Location);

	/**
	 * Finds the closest actor to another actor.
	 *
	 * @param Actors Array of actors to check.
	 * @param ReferenceActor The actor whose location is used as reference.
	 * @return The closest actor to the ReferenceActor, or nullptr if the array is empty or ReferenceActor is nullptr.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utilities|Actor", meta=(DefaultToSelf="ReferenceActor"))
	static AActor* GetClosestActor(const TArray<AActor*>& Actors, AActor* ReferenceActor);

private:
	// Templated function, not exposed to Blueprints
	template<typename Predicate>
	static AActor* GetClosestActorToActorFiltered(const TArray<AActor*>& Actors, AActor* ReferenceActor, Predicate Pred)
	{
		if (!ReferenceActor) return nullptr;

		AActor* ClosestActor = nullptr;
		float MinDistance = FLT_MAX;

		for (AActor* Actor : Actors)
		{
			if (Pred(Actor))
			{
				float Distance = (Actor->GetActorLocation() - ReferenceActor->GetActorLocation()).Size();
				if (Distance < MinDistance)
				{
					MinDistance = Distance;
					ClosestActor = Actor;
				}
			}
		}

		return ClosestActor;
	}
};
