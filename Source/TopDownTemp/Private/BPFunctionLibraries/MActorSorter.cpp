// Fill out your copyright notice in the Description page of Project Settings.

#include "BPFunctionLibraries/MActorSorter.h"

void UMActorSorter::SortActorsByDistance(const TArray<AActor*>& Actors, const FVector& Location, TArray<AActor*>& SortedActors)
{
	// Copy the array to the output array
	SortedActors = Actors;

	// Use a custom sorter with a lambda function
	SortedActors.Sort([Location](const AActor& A, const AActor& B) {
		return FVector::DistSquared(A.GetActorLocation(), Location) < FVector::DistSquared(B.GetActorLocation(), Location);
	});
}

AActor* UMActorSorter::GetClosestActorToLocation(const TArray<AActor*>& Actors, const FVector& Location)
{
	if (Actors.Num() == 0)
	{
		return nullptr;
	}

	AActor* ClosestActor = nullptr;
	float MinDistanceSquared = FLT_MAX;  // Initialize with the maximum possible float value

	for (AActor* Actor : Actors)
	{
		if (!Actor) continue;

		float DistanceSquared = FVector::DistSquared(Actor->GetActorLocation(), Location);
		if (DistanceSquared < MinDistanceSquared)
		{
			MinDistanceSquared = DistanceSquared;
			ClosestActor = Actor;
		}
	}

	return ClosestActor;
}

AActor* UMActorSorter::GetClosestActor(const TArray<AActor*>& Actors, AActor* ReferenceActor)
{
	if (!ReferenceActor || Actors.Num() == 0)
	{
		return nullptr;
	}

	return GetClosestActorToLocation(Actors, ReferenceActor->GetActorLocation());
}
