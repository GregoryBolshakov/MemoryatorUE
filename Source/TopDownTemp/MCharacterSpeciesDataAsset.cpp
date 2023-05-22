#include "MCharacterSpeciesDataAsset.h"

TArray<FName> UMCharacterSpeciesDataAsset::GetAllNamesByClass(TSubclassOf<AMCharacter> Class)
{
	TArray<FName> Result;
	for (const auto& [Name, CharacterData] : Data)
	{
		if (CharacterData.CharacterClass.Get()->IsChildOf(Class))
		{
			Result.Add(Name);
		}
	}
	return Result;
}
