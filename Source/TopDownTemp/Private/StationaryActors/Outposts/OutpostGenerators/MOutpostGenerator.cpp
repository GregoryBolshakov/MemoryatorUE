// Fill out your copyright notice in the Description page of Project Settings.

#include "MOutpostGenerator.h"

#include "Managers/SaveManager/MWorldSaveTypes.h"
#include "StationaryActors/Outposts/MOutpostHouse.h"

FMActorSaveData AMOutpostGenerator::GetSaveData() const
{
	auto MActorSD = Super::GetSaveData();
	MActorSD.ActorSaveData.MiscBool.Add("Generated", bGenerated);

	return MActorSD;
}

void AMOutpostGenerator::BeginLoadFromSD(const FMActorSaveData& MActorSD)
{
	Super::BeginLoadFromSD(MActorSD);
	bGenerated = MActorSD.ActorSaveData.MiscBool.FindChecked("Generated");
}
