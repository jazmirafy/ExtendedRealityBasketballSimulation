#include "XRBasketballSimCharacter.h"
#include "XRBasketballSimProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "Pickup.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputSubsystems.h"

// xrbasketballsimcharacter

AXRBasketballSimCharacter::AXRBasketballSimCharacter()
{
	// character does not start with a rifle
	bHasRifle = false;

	// set collision capsule size
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// create first-person camera
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// create first-person mesh (only visible to owner)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	// create holding component for picked up items
	HoldingComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HoldingComponent"));
	HoldingComponent->SetRelativeLocation(FVector(50.0f, 0.0f, 0.0f));
	HoldingComponent->SetupAttachment(FirstPersonCameraComponent);

	// initialize interaction states
	CurrentItem = NULL;
	bCanMove = true;
	bInspecting = false;
}

// called when the game starts
void AXRBasketballSimCharacter::BeginPlay()
{
	Super::BeginPlay();

	// add enhanced input mapping contexts
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
				PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
			Subsystem->AddMappingContext(GameplayMappingContext, 1);
		}
	}

	// cache camera pitch limits
	PitchMax = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMax;
	PitchMin = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMin;
}

// called every frame
void AXRBasketballSimCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// set up line trace from camera
	Start = FirstPersonCameraComponent->GetComponentLocation();
	ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	End = (ForwardVector * 300.0f) + Start;

	// draw debug line for interaction trace
	DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, 1, 0, 1);

	// detect pickup objects when not holding anything
	if (!bHoldingItem)
	{
		if (GetWorld()->LineTraceSingleByChannel(
			Hit, Start, End, ECC_Visibility,
			DefaultComponentQueryParams, DefaultResponseParams))
		{
			if (Hit.GetActor()->GetClass()->IsChildOf(APickup::StaticClass()))
			{
				CurrentItem = Cast<APickup>(Hit.GetActor());
			}
		}
		else
		{
			CurrentItem = NULL;
		}
	}

	// camera behavior while inspecting or holding items
	if (bInspecting)
	{
		if (bHoldingItem)
		{
			// lock movement and zoom in for inspection
			bCanMove = false;
			FirstPersonCameraComponent->SetFieldOfView(
				FMath::Lerp(FirstPersonCameraComponent->FieldOfView, 70.0f, 0.1f));
			HoldingComponent->SetRelativeLocation(FVector(0.0f, 50.0f, 50.0f));

			// allow full pitch rotation
			GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMax = 179.9f;
			GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMin = -179.9f;
		}
		else
		{
			// zoom slightly when inspecting without holding
			FirstPersonCameraComponent->SetFieldOfView(
				FMath::Lerp(FirstPersonCameraComponent->FieldOfView, 45.0f, 0.1f));
		}
	}
	else
	{
		// default gameplay fov
		FirstPersonCameraComponent->SetFieldOfView(
			FMath::Lerp(FirstPersonCameraComponent->FieldOfView, 90.0f, 0.1f));

		// reset holding position when not inspecting
		if (bHoldingItem)
		{
			HoldingComponent->SetRelativeLocation(FVector(50.0f, 0.0f, 0.0f));
		}
	}

	// Force camera to respect controller rotation in VR preview (CAVE fix)
	if (Controller && !bInspecting && FirstPersonCameraComponent->bUsePawnControlRotation)
	{
		FRotator ControlRot = Controller->GetControlRotation();
		FRotator CurrentRot = FirstPersonCameraComponent->GetComponentRotation();

		// Only override if XR system has deviated our pitch
		FRotator ForcedRot = FRotator(ControlRot.Pitch, ControlRot.Yaw, CurrentRot.Roll);
		FirstPersonCameraComponent->SetWorldRotation(ForcedRot);
	}
}

// input

void AXRBasketballSimCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent =
		CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// jump input
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// movement input
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AXRBasketballSimCharacter::Move);

		// camera look input
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AXRBasketballSimCharacter::Look);

		// pickup / throw input
		EnhancedInputComponent->BindAction(ActionAction, ETriggerEvent::Triggered, this, &AXRBasketballSimCharacter::Action);

		// inspect input
		EnhancedInputComponent->BindAction(InspectAction, ETriggerEvent::Triggered, this, &AXRBasketballSimCharacter::Inspect);
		EnhancedInputComponent->BindAction(InspectAction, ETriggerEvent::Completed, this, &AXRBasketballSimCharacter::StopInspecting);
	}
}

// movement logic
void AXRBasketballSimCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller && bCanMove && !bInspecting)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

// camera look logic
void AXRBasketballSimCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller && bCanMove && !bInspecting)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// action button handler
void AXRBasketballSimCharacter::Action()
{
	if (CurrentItem && !bInspecting)
	{
		ToggleItemPickup();
	}
}

// begin inspect mode
void AXRBasketballSimCharacter::Inspect()
{
	if (bHoldingItem)
	{
		LastRotation = GetControlRotation();
		ToggleMovement();
	}
	else
	{
		bInspecting = true;
	}
}

// end inspect mode
void AXRBasketballSimCharacter::StopInspecting()
{
	if (bInspecting && bHoldingItem)
	{
		GetController()->SetControlRotation(LastRotation);
		GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMax = PitchMax;
		GetWorld()->GetFirstPlayerController()->PlayerCameraManager->ViewPitchMin = PitchMin;
		ToggleMovement();
	}
	else
	{
		bInspecting = false;
	}
}

// toggles movement and camera control
void AXRBasketballSimCharacter::ToggleMovement()
{
	bInspecting = !bInspecting;
	bCanMove = !bInspecting;
	FirstPersonCameraComponent->bUsePawnControlRotation = !FirstPersonCameraComponent->bUsePawnControlRotation;
	bUseControllerRotationYaw = !bUseControllerRotationYaw;
}

// handles pickup state changes
void AXRBasketballSimCharacter::ToggleItemPickup()
{
	if (CurrentItem)
	{
		bHoldingItem = !bHoldingItem;
		CurrentItem->Pickup();

		if (!bHoldingItem)
		{
			CurrentItem = NULL;
		}
	}
}

void AXRBasketballSimCharacter::SetHasRifle(bool bNewHasRifle)
{
	bHasRifle = bNewHasRifle;
}

bool AXRBasketballSimCharacter::GetHasRifle()
{
	return bHasRifle;
}
