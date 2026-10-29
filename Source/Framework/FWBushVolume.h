#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FWBushVolume.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

/**
 * Invisible trigger placed over a bush in the level. When the local pawn enters/leaves it,
 * AFWPlayerController reports the bush to the server, which hides that player from enemies
 * outside the same bush. Scale/rotate the box to cover the bush mesh.
 */
UCLASS()
class FRAMEWORK_API AFWBushVolume : public AActor
{
	GENERATED_BODY()

public:
	AFWBushVolume();

	/**
	 * 1-based id, identical on every client that loads the same map (bush volumes sorted by actor name).
	 * 0 is reserved for "not in a bush" on the wire, and ids above 255 don't fit the packet.
	 */
	int32 GetBushId() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Bush")
	TObjectPtr<UBoxComponent> Area;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void NotifyLocalController(AActor* OtherActor, bool bEntered) const;

	mutable int32 CachedBushId = 0;
};
