// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacter.h"

#include "SGameMode.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/InputComponent.h"
#include "ScoundrelCorp/Public/SWeapon.h"
#include "Components/CapsuleComponent.h"
#include "ScoundrelCorp/ScoundrelCorp.h"
#include "ScoundrelCorp/Components/SHealthComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASCharacter::ASCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	WeaponAttachSocketName = "WeaponSocket";
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultFOV = CameraComp->FieldOfView;

	lastAbilityTime = 0.0f;
	
	if (GetLocalRole() == ROLE_Authority) {
		//spawn a default weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentWeapon) {
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
		}

		HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);
	}
}

void ASCharacter::MoveForward(float value)
{
	AddMovementInput(GetActorForwardVector() * value);
}

void ASCharacter::MoveRight(float value)
{
	AddMovementInput(GetActorRightVector() * value);
}

void ASCharacter::BeginCrouch()
{
	Crouch();
}

void ASCharacter::EndCrouch()
{
	UnCrouch();
}

void ASCharacter::HandleZoom(float DeltaTime)
{
	if(CurrentWeapon == nullptr)
		return;
	
	const float TargetFOV = bWantsToZoom ? CurrentWeapon->GetZoomedFOV() : DefaultFOV;

	const float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, CurrentWeapon->GetZoomSpeed());

	CameraComp->SetFieldOfView(NewFOV);
}

void ASCharacter::OnRep_Ability()
{
	// call animation stuff
}


// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleZoom(DeltaTime);	
}

void ASCharacter::Zoom() {
	bWantsToZoom = true;
}

void ASCharacter::Unzoom() {
	bWantsToZoom = false;
}

void ASCharacter::StartFire()
{
	if (CurrentWeapon) {
		CurrentWeapon->StartFire();
	}
}

void ASCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void ASCharacter::StartReload()
{
	if(CurrentWeapon)
	{
		CurrentWeapon->StartReload();
	}
}

void ASCharacter::StopReload()
{
	if(CurrentWeapon)
		CurrentWeapon->StopReload();
}

/** Function used to kickoff ability, the PERFORMABILITY function should contain actual ability logic for this character.*/
void ASCharacter::StartAbility()
{
	if(GetLocalRole() < ROLE_Authority)
		ServerStartAbility();
	
	if(GetWorld()->TimeSeconds - lastAbilityTime >= 0.0f)
	{
		//function used to replicate this will kick off the animations, need to run it locally for the owner though.
		// I'm not going to have ability use stop shooting/reloading, so we don't care about the value of this we just want to trigger the repNotify.
		bPendingAbility = !bPendingAbility;
		PerformAbility();
	}

	GetWorldTimerManager().SetTimer(TimerHandle_AbilityTime, this, &ASCharacter::CompleteAbility, AbilityCooldown, false);
}

void ASCharacter::CompleteAbility()
{
	lastAbilityTime = GetWorld()->TimeSeconds;
}

void ASCharacter::ServerStartAbility_Implementation()
{
	StartAbility();
}

bool ASCharacter::ServerStartAbility_Validate()
{
	return true;
}

void ASCharacter::OnHealthChanged(USHealthComponent * OwningHealthComp, float Health, float HealthDelta, const UDamageType * DamageType, AController * InstigatedBy, AActor * DamageCauser)
{
	if (Health <= 0.0f && !bDied) {
		//Die!

		bDied = true;

		GetMovementComponent()->StopMovementImmediately();

		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		APlayerController* PC = Cast<APlayerController>(GetController());
		
		DetachFromControllerPendingDestroy();

		SetLifeSpan(10.0f);

		if(GetLocalRole() == ROLE_Authority)
		{
			ASGameMode* GM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());

			GM->RestartDeadPlayer(PC);		
		}
		
	}
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::Zoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::Unzoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASCharacter::StartReload);
	PlayerInputComponent->BindAction("Ability", IE_Pressed, this, &ASCharacter::StartAbility);
}

FVector ASCharacter::GetPawnViewLocation() const
{
	if (CameraComp) {
		return CameraComp->GetComponentLocation();
	}

	return Super::GetPawnViewLocation();
}

void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, bDied);
}