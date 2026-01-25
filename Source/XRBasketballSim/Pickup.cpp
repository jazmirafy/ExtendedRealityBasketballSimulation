#include "Camera/CameraComponent.h"
#include "Pickup.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

// sets default values for the pickup actor
APickup::APickup()
{
	// enable ticking every frame
	PrimaryActorTick.bCanEverTick = true;

	// create mesh component and enable physics
	MyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MyMesh"));
	MyMesh->SetSimulatePhysics(true);

	// set mesh as root component
	RootComponent = MyMesh;

	// initialize pickup state
	bHolding = false;
	bGravity = true;
}

// called when the game starts or when spawned
void APickup::BeginPlay()
{
	Super::BeginPlay();

	// get the player character
	MyCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);

	// get the player camera component
	PlayerCamera = MyCharacter->FindComponentByClass<UCameraComponent>();

	// find the holding component attached to the player
	TArray<USceneComponent*> Components;
	MyCharacter->GetComponents(Components);

	if (Components.Num() > 0)
	{
		for (auto& Comp : Components)
		{
			// match by name so we know where to attach held objects
			if (Comp->GetName() == "HoldingComponent")
			{
				HoldingComp = Cast<USceneComponent>(Comp);
			}
		}
	}
}

// called every frame
void APickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// snap pickup to holding component when held
	if (bHolding && HoldingComp)
	{
		SetActorLocationAndRotation(
			HoldingComp->GetComponentLocation(),
			HoldingComp->GetComponentRotation()
		);
	}
}

// rotates the pickup to match player control rotation
void APickup::RotateActor()
{
	ControlRotation = GetWorld()->GetFirstPlayerController()->GetControlRotation();
	SetActorRotation(FQuat(ControlRotation));
}

// toggles pickup and throw behavior
void APickup::Pickup()
{
	// toggle holding and gravity states
	bHolding = !bHolding;
	bGravity = !bGravity;

	// update physics and collision based on holding state
	MyMesh->SetEnableGravity(bGravity);
	MyMesh->SetSimulatePhysics(bHolding ? false : true);
	MyMesh->SetCollisionEnabled(
		bHolding ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics
	);

	// apply impulse forward when released
	if (!bHolding)
	{
		ForwardVector = PlayerCamera->GetForwardVector();
		MyMesh->AddImpulse(
			ForwardVector * ForceAmount * MyMesh->GetMass()
		);
	}
}

