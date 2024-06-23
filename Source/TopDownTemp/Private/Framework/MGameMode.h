#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MGameMode.generated.h"

class AMWorldGenerator;
class UMMetadataManager;
class UMDropManager;
class UMReputationManager;
class UMExperienceManager;
class UMSaveManager;
class AMCommunicationManager;
class UMRoadManager;

UCLASS(minimalapi)
class AMGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	static inline AMGameMode* GetAMGameMode (const UObject* Caller);
	// Getters for managers
	static inline AMWorldGenerator* GetWorldGenerator(const UObject* Caller);
	AMWorldGenerator* GetWorldGenerator() const { return WorldGenerator; }

	static inline UMMetadataManager* GetMetadataManager(const UObject* Caller);
	UMMetadataManager* GetMetadataManager() const { return MetadataManager; }

	static inline UMDropManager* GetDropManager(const UObject* Caller);
	UFUNCTION(BlueprintCallable)
	UMDropManager* GetDropManager() const { return DropManager; }

	static inline UMReputationManager* GetReputationManager(const UObject* Caller);
	UFUNCTION(BlueprintCallable)
	UMReputationManager* GetReputationManager() const { return ReputationManager; }

	static inline UMExperienceManager* GetExperienceManager(const UObject* Caller);
	UFUNCTION(BlueprintCallable)
	UMExperienceManager* GetExperienceManager() const { return ExperienceManager; }

	static inline UMSaveManager* GetSaveManager(const UObject* Caller);
	UMSaveManager* GetSaveManager() const { return SaveManager; }

	static inline AMCommunicationManager* GetCommunicationManager(const UObject* Caller);
	UFUNCTION(BlueprintCallable)
	AMCommunicationManager* GetCommunicationManager() const { return CommunicationManager; }

	static inline UMRoadManager* GetRoadManager(const UObject* Caller);
	UMRoadManager* GetRoadManager() const { return RoadManager; }

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override; // TODO: think of moving to AMGameState or something...

	virtual void BeginPlay() override;

	/** Temporarily switch to offline mode, continue the game and keep checking the connection */
	UFUNCTION(BlueprintCallable)
	void GoOffline();

protected: // Manager classes // TODO: Think of making as Soft Pointers
	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator)
	TSubclassOf<AMWorldGenerator> WorldGeneratorBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<UMDropManager> DropManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<UMReputationManager> ReputationManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator)
	TSubclassOf<UMExperienceManager> ExperienceManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator)
	TSubclassOf<UMRoadManager> RoadManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator)
	TSubclassOf<UMSaveManager> SaveManagerBPClass;

	UPROPERTY(EditDefaultsOnly, Category=MWorldGenerator, meta=(AllowPrivateAccess=true))
	TSubclassOf<AMCommunicationManager> CommunicationManagerBPClass;

private:
	/** Those connections received at PostLogin() but before BeginPlay() have to wait until BeginPlay so all systems are able to process them. */
	TQueue<APlayerController*> ConnectionQueue;

private: // Managers
	void InitializeManagers();

	UPROPERTY()
	AMWorldGenerator* WorldGenerator = nullptr;

	/** Matches actor names/Uids/etc. with their metadata.
	* Once a world is loaded, ActorMetadata is available only for actors from visited blocks. */
	UPROPERTY()
	UMMetadataManager* MetadataManager = nullptr;

	UPROPERTY()
	UMDropManager* DropManager = nullptr;

	UPROPERTY()
	UMReputationManager* ReputationManager = nullptr;

	UPROPERTY()
	UMExperienceManager* ExperienceManager = nullptr;

	UPROPERTY()
	UMSaveManager* SaveManager = nullptr;

	//TODO: Fix needed: items disappear when game crashes/closes during a trade after items were moved to the widget
	UPROPERTY()
	AMCommunicationManager* CommunicationManager = nullptr;

	UPROPERTY()
	UMRoadManager* RoadManager = nullptr;
};

#if CPP
#include "MGameMode.inl"
#endif