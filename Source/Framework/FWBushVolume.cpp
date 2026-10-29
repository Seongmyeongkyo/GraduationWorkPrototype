#include "FWBushVolume.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "FWPlayerController.h"

AFWBushVolume::AFWBushVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	SetRootComponent(Area);
	// Default matches one bush ellipse in the Round01 layout (2.8m x 1.35m radii).
	Area->SetBoxExtent(FVector(280.f, 135.f, 150.f));
	// Overlap-only: doesn't block movement, clicks or the cursor trace.
	Area->SetCollisionProfileName(TEXT("Trigger"));
	Area->SetGenerateOverlapEvents(true);
}

void AFWBushVolume::BeginPlay()
{
	Area->OnComponentBeginOverlap.AddDynamic(this, &AFWBushVolume::HandleBeginOverlap);
	Area->OnComponentEndOverlap.AddDynamic(this, &AFWBushVolume::HandleEndOverlap);

	Super::BeginPlay();
}

int32 AFWBushVolume::GetBushId() const
{
	if (CachedBushId == 0 && GetWorld())
	{
		TArray<FString> Names;
		for (TActorIterator<AFWBushVolume> It(GetWorld()); It; ++It)
		{
			Names.Add(It->GetName());
		}
		Names.Sort();
		CachedBushId = Names.IndexOfByKey(GetName()) + 1;
	}
	return CachedBushId;
}

void AFWBushVolume::HandleBeginOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	NotifyLocalController(OtherActor, true);
}

void AFWBushVolume::HandleEndOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
	NotifyLocalController(OtherActor, false);
}

void AFWBushVolume::NotifyLocalController(AActor* OtherActor, bool bEntered) const
{
	// Only our own pawn reports - remote avatars are hidden/shown by the server instead.
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}
	if (AFWPlayerController* PC = Cast<AFWPlayerController>(Pawn->GetController()))
	{
		PC->OnLocalPawnBushOverlap(GetBushId(), bEntered);
	}
}
