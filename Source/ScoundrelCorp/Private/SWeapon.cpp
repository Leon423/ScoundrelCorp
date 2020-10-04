// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ScoundrelCorp/ScoundrelCorp.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "ScoundrelCorp/Public/SCharacter.h"

int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
    TEXT("COOP.DebugWeapons"),
    DebugWeaponDrawing,
    TEXT("Draw Debug Lines for Weapons"),
    ECVF_Cheat);

// Sets default values
ASWeapon::ASWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";

	bPendingReload = false;
	bIsFiring = false;
	
	BaseDamage = 20.0f;
	HeadshotDamageMultiplier = 2.0f;
	BulletSpread = 2.0f;
	RateOfFire = 600;
	ReloadTime = 1.0f;

	CurrentAmmo = 0;
	CurrentAmmoInMag = 0;
	
	SetReplicates(true);

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

void ASWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if(InitialMags > 0)
	{
		CurrentAmmoInMag = AmmoPerMag;
		CurrentAmmo = AmmoPerMag * InitialMags;
	}
}

// Called when the game starts or when spawned
void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60 / RateOfFire;
}

void ASWeapon::OnRep_HitScanTrace()
{
	// play cosmetic FX
	PlayFireEffects(HitScanTrace.TraceTo);

	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);

}

bool ASWeapon::CanFire() const
{

	return bPendingReload == false && CurrentAmmoInMag > 0;
}

bool ASWeapon::CanReload() const
{
	bool bGotAmmo = (CurrentAmmoInMag < AmmoPerMag) && (CurrentAmmo - CurrentAmmoInMag > 0);
	//we should always be able to cancel what we are doing into a reload.
	return bGotAmmo == true;
}

void ASWeapon::OnRep_Reload()
{
	// play effects here (probably gotta check that it is true and not false)
	
}

void ASWeapon::Fire()
{
	// trace the world, from pawn eyes to crosshair position
	if (GetLocalRole() < ROLE_Authority) {
		ServerFire();
	}

	if(!CanFire())
		return;


	AActor* MyOwner = GetOwner();

	if (MyOwner) 
	{
		FVector EyeLocation;
		FRotator EyeRotation;

		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();

		// bullet spread 
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		FVector TraceEnd = EyeLocation + (ShotDirection * 10000);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		FHitResult Hit;

		// particle "Target" parameter
		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;
		

		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
		{
			// blocking hit! Process damage
			AActor* HitActor = Hit.GetActor();

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = (SurfaceType == SURFACE_FLESHVULNERABLE) ? BaseDamage * HeadshotDamageMultiplier : BaseDamage;


			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

			PlayImpactEffects(SurfaceType, Hit.ImpactPoint);

			TracerEndPoint = Hit.ImpactPoint;

			
		}

		if (DebugWeaponDrawing > 0) {
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::Red, false, 1.0, 0, 1.0f);
		}

		PlayFireEffects(TracerEndPoint);


		if (GetLocalRole() == ROLE_Authority) {
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;

			CurrentAmmoInMag--;
			CurrentAmmo--;
		}

		if(Cast<ASCharacter>(MyOwner)->IsLocallyControlled() || GetLocalRole() == ROLE_Authority)
		{
			
		}

		LastFireTime = GetWorld()->TimeSeconds;
	}
}

void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::StartFire()
{

	float FirstDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void ASWeapon::Reload()
{

	if(GetLocalRole() < ROLE_Authority)
		ServerReload();
	
	if(!CanReload())
	{
		//just return if we are at max ammo, this could cause the user/server to have different results if something happens
		// like the user seeing max ammo on their screen, but the server thinks they have less ammo.
		// but this should be fine.
		return;
	}
	//function used to replicate this will kick off the animations, now just set the "complete reload" timer for the ammo to be added
	// the animations will need to be played locally though, as it doesn't repnotify to the owner.
	bPendingReload = true;

	GetWorldTimerManager().SetTimer(TimerHandle_ReloadTime, this, &ASWeapon::CompleteReload, ReloadTime, false);
}

void ASWeapon::CompleteReload()
{
	//get the amount of ammo we are adding to the magazine
	int32 ClipDelta = FMath::Min(AmmoPerMag - CurrentAmmoInMag, CurrentAmmo - CurrentAmmoInMag);

	if(ClipDelta > 0)
		CurrentAmmoInMag += ClipDelta;

	bPendingReload = false;
}

void ASWeapon::StartReload()
{
	Reload();
}

void ASWeapon::StopReload()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_ReloadTime);
}

void ASWeapon::ServerReload_Implementation()
{
	Reload();
}

bool ASWeapon::ServerReload_Validate()
{
	return true;
}

void ASWeapon::PlayFireEffects(FVector TraceEnd)
{
	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}

	if (TracerEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);

		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TracerTargetName, TraceEnd);
		}
	}
	
	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner)
	{
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());

		if (PC) {
			PC->ClientPlayCameraShake(FireCamShake);
		}
	}
}

void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
	UParticleSystem* SelectedEffect = nullptr;

	switch (SurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
	case SURFACE_FLESHVULNERABLE:
		SelectedEffect = FleshImpactEffect;
		break;
	default:
		SelectedEffect = DefaultImpactEffect;
		break;
	}

	if (SelectedEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
	}
}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
	DOREPLIFETIME_CONDITION( ASWeapon, bPendingReload,	COND_SkipOwner );
	DOREPLIFETIME_CONDITION( ASWeapon, CurrentAmmo,		COND_OwnerOnly );
	DOREPLIFETIME_CONDITION( ASWeapon, CurrentAmmoInMag, COND_OwnerOnly );
}

