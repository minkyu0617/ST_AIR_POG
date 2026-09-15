// A cloud-top shop. Fly through slowly enough and the transaction happens - no landing,
// no full stop (docs/05 §5.2). This is the core "detour or not" decision made physical.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/Types/FPGTypes.h"
#include "FPGPoiStation.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class AFPGAircraftPawn;

UCLASS()
class FPG_API AFPGPoiStation : public AActor
{
	GENERATED_BODY()

public:
	AFPGPoiStation();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "FPG|POI") FName GetPoiId() const { return PoiId; }
	UFUNCTION(BlueprintPure, Category = "FPG|POI") float GetDockProgress() const;

protected:
	/** Row name in DT_Poi. */
	UPROPERTY(EditAnywhere, Category = "FPG|POI") FName PoiId = TEXT("POI_REPAIR_BAY");

	UPROPERTY(VisibleAnywhere, Category = "FPG|POI") TObjectPtr<USphereComponent> DockZone;
	UPROPERTY(VisibleAnywhere, Category = "FPG|POI") TObjectPtr<UStaticMeshComponent> Marker;

private:
	UFUNCTION()
	void OnDockZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDockZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void CompleteTransaction(AFPGAircraftPawn* Aircraft);

	UPROPERTY(Transient) TObjectPtr<AFPGAircraftPawn> DockingAircraft;
	float DockElapsed = 0.f;
	int32 VisitCount = 0;
};
