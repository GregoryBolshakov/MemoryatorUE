#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "M2DRepresentationBlueprintLibrary.generated.h"

UCLASS()
class TOPDOWNTEMP_API UM2DRepresentationBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category=M2DRepresentation)
	static float GetCameraDeflectionAngle(const UObject* WorldContextObject, const FVector GazeDirection);

	UFUNCTION(BlueprintCallable, Category=M2DRepresentation)
	static float GetDeflectionAngle(const FVector VectorA, const FVector VectorB);

	UFUNCTION(BlueprintCallable, Category=M2DRepresentation)
	static FRotator GetRotationTowardVector(const FVector& DirectionVector);
};

inline float UM2DRepresentationBlueprintLibrary::GetCameraDeflectionAngle(const UObject* WorldContextObject,
	FVector GazeDirection)
{
	const UWorld* pWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!pWorld)
	{
		return 0.f;
	}

	auto CameraVector = pWorld->GetFirstPlayerController()->PlayerCameraManager->GetActorForwardVector();
	CameraVector.Z = 0;

	GazeDirection.Z = 0;

	const FVector CrossProduct = FVector::CrossProduct(CameraVector, GazeDirection);
	float Angle = FMath::RadiansToDegrees(FMath::Atan2(CrossProduct.Size(), FVector::DotProduct(CameraVector, GazeDirection)));
	const FVector UpVector(0.f, 0.f, 1.f);
	float Sign = FVector::DotProduct(UpVector, CrossProduct) < 0.f ? -1.f : 1.f;
	Angle *= Sign;

	return Angle;
}

inline float UM2DRepresentationBlueprintLibrary::GetDeflectionAngle(FVector VectorA, FVector VectorB)
{
	VectorA.Z = 0;
	VectorB.Z = 0;

	const FVector CrossProduct = FVector::CrossProduct(VectorA, VectorB);
	float Angle = FMath::RadiansToDegrees(FMath::Atan2(CrossProduct.Size(), FVector::DotProduct(VectorA, VectorB)));
	const FVector UpVector(0.f, 0.f, 1.f);
	float Sign = FVector::DotProduct(UpVector, CrossProduct) < 0.f ? -1.f : 1.f;
	Angle *= Sign;

	return Angle;
}

inline FRotator UM2DRepresentationBlueprintLibrary::GetRotationTowardVector(const FVector& DirectionVector)
{
	return FRotationMatrix::MakeFromX(DirectionVector).Rotator();
}