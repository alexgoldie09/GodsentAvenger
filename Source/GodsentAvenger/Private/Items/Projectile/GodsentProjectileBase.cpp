// Alex Goldberg rights reserved

#include "Items/Projectile/GodsentProjectileBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GodsentFunctionLibrary.h"
#include "GodsentGameplayTags.h"

AGodsentProjectileBase::AGodsentProjectileBase()
{
	// Ticks only while homing
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Walls and floors stop it; pawns are overlapped so an invincible (rolling) player can be passed through.
	// Other projectiles and weapons (WorldDynamic) are ignored, so a volley never collides with itself.
	ProjectileCollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileCollisionSphere"));
	SetRootComponent(ProjectileCollisionSphere);
	ProjectileCollisionSphere->InitSphereRadius(16.f);
	ProjectileCollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProjectileCollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	ProjectileCollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	ProjectileCollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	ProjectileCollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ProjectileCollisionSphere->SetGenerateOverlapEvents(true);
	ProjectileCollisionSphere->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnProjectileHit);
	ProjectileCollisionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnProjectileBeginOverlap);

	ProjectileNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ProjectileNiagaraComponent"));
	ProjectileNiagaraComponent->SetupAttachment(GetRootComponent());

	// Slow by design: dodgeable from range
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->InitialSpeed = 450.f;
	ProjectileMovementComponent->MaxSpeed = 700.f;
	ProjectileMovementComponent->Velocity = FVector(1.f, 0.f, 0.f); // Along the spawn rotation
	ProjectileMovementComponent->bInitialVelocityInLocalSpace = true;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;
	ProjectileMovementComponent->bIsHomingProjectile = false;

	InitialLifeSpan = 6.f;
}

void AGodsentProjectileBase::InitProjectile(const FGameplayEffectSpecHandle& InDamageEffectSpecHandle, AActor* InHomingTarget)
{
	ProjectileDamageEffectSpecHandle = InDamageEffectSpecHandle;
	HomingTarget = InHomingTarget;
}

void AGodsentProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	// Never collide with the caster while leaving it
	if (APawn* InstigatorPawn = GetInstigator())
		ProjectileCollisionSphere->IgnoreActorWhenMoving(InstigatorPawn, true);

	if (!HomingTarget)
		return;

	if (HomingDelay > 0.f)
		GetWorldTimerManager().SetTimer(HomingStartTimerHandle, this, &ThisClass::StartHoming, HomingDelay, false);
	else
		StartHoming();
}

void AGodsentProjectileBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsHoming)
		return;

	HomingElapsedTime += DeltaSeconds;

	if (!IsValid(HomingTarget) || HomingElapsedTime >= MaxHomingDuration)
	{
		StopHoming();
		return;
	}

	const FVector ToTarget = HomingTarget->GetActorLocation() - GetActorLocation();

	// Close enough, or already flown past: commit to the current line.
	// "Past" is judged on the ground plane only, so a projectile still rising above a nearby target doesn't count.
	const bool bWithinCommitDistance = ToTarget.SizeSquared() <= FMath::Square(HomingCommitDistance);
	const bool bHasPassedTarget = HomingElapsedTime > 0.25f &&
		FVector::DotProduct(ProjectileMovementComponent->Velocity.GetSafeNormal2D(), ToTarget.GetSafeNormal2D()) < 0.f;

	if (bWithinCommitDistance || bHasPassedTarget)
		StopHoming();
}

void AGodsentProjectileBase::LifeSpanExpired()
{
	if (!bHasFinished)
	{
		bHasFinished = true;
		BP_OnProjectileImpact(GetActorLocation(), EGodsentProjectileImpact::Fizzled);
	}

	Super::LifeSpanExpired();
}

void AGodsentProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only world geometry blocks it (pawns are overlaps)
	FinishProjectile(Hit.ImpactPoint, EGodsentProjectileImpact::World);
}

void AGodsentProjectileBase::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* HitPawn = Cast<APawn>(OtherActor);

	if (!HitPawn || bHasFinished)
		return;

	HandlePawnHit(HitPawn);
}

/// Block, damage and guard break against a hostile pawn. Returns false if the projectile should keep flying.
bool AGodsentProjectileBase::HandlePawnHit(APawn* HitPawn)
{
	APawn* InstigatorPawn = GetInstigator();

	// Caster died and was cleaned up: the spell dies with it
	if (!IsValid(InstigatorPawn))
	{
		FinishProjectile(GetActorLocation(), EGodsentProjectileImpact::Fizzled);
		return true;
	}

	if (HitPawn == InstigatorPawn || !UGodsentFunctionLibrary::IsTargetPawnHostile(InstigatorPawn, HitPawn))
		return false;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitPawn);

	if (!TargetASC)
		return false;

	// Already dead, or mid-dodge with invincibility: fly straight through
	if (UGodsentFunctionLibrary::NativeDoesActorHaveTag(HitPawn, GodsentGameplayTags::Shared_Status_Dead) ||
		UGodsentFunctionLibrary::NativeDoesActorHaveTag(HitPawn, GodsentGameplayTags::Shared_Status_Invincible))
		return false;

	// Block direction is judged from where the projectile arrives, not from where the caster stands
	const EGodsentBlockResult BlockResult = UGodsentFunctionLibrary::NativeResolveBlock(this, HitPawn, !bCanBeBlocked, BlockFacingThreshold);

	// Block and parry events name the caster, so a parry can stagger it like a melee parry does
	if (UGodsentFunctionLibrary::NativeHandleBlockResult(InstigatorPawn, HitPawn, BlockResult))
	{
		FinishProjectile(GetActorLocation(), EGodsentProjectileImpact::Blocked);
		return true;
	}

	const bool bGuardBroken = UGodsentFunctionLibrary::NativeBreakGuardIfBlocking(HitPawn, BlockResult);

	if (ProjectileDamageEffectSpecHandle.IsValid())
		TargetASC->ApplyGameplayEffectSpecToSelf(*ProjectileDamageEffectSpecHandle.Data.Get());

	if (bGuardBroken)
		UGodsentFunctionLibrary::NativeSendGuardBreakStagger(InstigatorPawn, HitPawn);

	FinishProjectile(GetActorLocation(), EGodsentProjectileImpact::Pawn);
	return true;
}

void AGodsentProjectileBase::StartHoming()
{
	if (!IsValid(HomingTarget) || bHasFinished)
		return;

	ProjectileMovementComponent->HomingTargetComponent = HomingTarget->GetRootComponent();
	ProjectileMovementComponent->HomingAccelerationMagnitude = HomingAcceleration;
	ProjectileMovementComponent->bIsHomingProjectile = true;

	HomingElapsedTime = 0.f;
	bIsHoming = true;
	SetActorTickEnabled(true);
}

void AGodsentProjectileBase::StopHoming()
{
	ProjectileMovementComponent->bIsHomingProjectile = false;
	bIsHoming = false;
	SetActorTickEnabled(false);
}

/// Plays the impact event once and destroys the projectile
void AGodsentProjectileBase::FinishProjectile(const FVector& ImpactLocation, EGodsentProjectileImpact ImpactType)
{
	if (bHasFinished)
		return;

	bHasFinished = true;
	GetWorldTimerManager().ClearTimer(HomingStartTimerHandle);

	BP_OnProjectileImpact(ImpactLocation, ImpactType);
	Destroy();
}
