#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "XRBasketballSimCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;

// first person character with pickup and inspect interaction
UCLASS(config=Game)
class AXRBasketballSimCharacter : public ACharacter
{
	GENERATED_BODY()

	// first person arms mesh
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	// camera used for interaction
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	// default input mapping
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	// gameplay input mapping
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* GameplayMappingContext;

	// jump input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	// movement input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	// pickup and throw input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ActionAction;

	// inspect input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InspectAction;

	// anchor for held items
	UPROPERTY(EditAnywhere)
	class USceneComponent* HoldingComponent;

public:
	// initializes character components
	AXRBasketballSimCharacter();

protected:
	// sets up input and camera limits
	virtual void BeginPlay();

	// updates interaction logic
	virtual void Tick(float DeltaSeconds) override;

public:
	// look input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	// animation state flag
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon)
	bool bHasRifle;

	// sets animation state
	UFUNCTION(BlueprintCallable, Category = Weapon)
	void SetHasRifle(bool bNewHasRifle);

	// gets animation state
	UFUNCTION(BlueprintCallable, Category = Weapon)
	bool GetHasRifle();

	// currently targeted pickup
	UPROPERTY(EditAnywhere)
	class APickup* CurrentItem;

	// movement enabled flag
	bool bCanMove;

	// holding state flag
	bool bHoldingItem;

	// inspect mode flag
	bool bInspecting;

	// cached camera pitch limits
	float PitchMax;
	float PitchMin;

	// cached interaction data
	FVector HoldingComp;
	FRotator LastRotation;

	// line trace data
	FVector Start;
	FVector ForwardVector;
	FVector End;

	// last trace hit
	FHitResult Hit;

	// trace parameters
	FComponentQueryParams DefaultComponentQueryParams;
	FCollisionResponseParams DefaultResponseParams;

protected:
	// movement input handler
	void Move(const FInputActionValue& Value);

	// look input handler
	void Look(const FInputActionValue& Value);

	// enters inspect mode
	void Inspect();

	// exits inspect mode
	void StopInspecting();

	// pickup action handler
	void Action();

	// toggles movement state
	void ToggleMovement();

	// toggles pickup state
	void ToggleItemPickup();

protected:
	// binds input actions
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:
	// returns first person mesh
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

	// returns camera component
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
};


