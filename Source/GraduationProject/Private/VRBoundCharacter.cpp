// Fill out your copyright notice in the Description page of Project Settings.

#include "GraduationProject.h"
#include "VRBoundCharacter.h"
#include <Animation/AnimInstance.h>

FVector GetXYDir(const FVector &in)
{
	FVector out = in;
	out.Z = 0.0f;
	out.Normalize();
	return out;
}

float GetXYAngle(const FVector &a, const FVector &b)
{
	FVector ap = GetXYDir(a);
	FVector bp = GetXYDir(b);
	FVector rotVec = FVector::CrossProduct(ap, bp);
	float angle = asin(fabs(rotVec.Z)) * 180.0f / PI;
	if (FVector::DotProduct(ap, bp) < 0.0f)
	{
		angle = 180.0f - angle;
	}
	float sign = (rotVec.Z > 0.0f) ? 1.0f : -1.0f;
	angle *= sign;
	return angle;
}

// Sets default values
AVRBoundCharacter::AVRBoundCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	InterpSpeed = 500.0f;
}

// Called when the game starts or when spawned
void AVRBoundCharacter::BeginPlay()
{
	Super::BeginPlay();

	USkeletalMeshComponent *SkeletalMeshComponent = GetMesh();
	HeadSocket = SkeletalMeshComponent->GetSocketByName(TEXT("HeadSocket"));

	SkeletalMeshComponentOriginalHeight = SkeletalMeshComponent->GetRelativeTransform().GetLocation().Z;

	FTransform actorTransform = GetActorTransform();
	TransformZScale = actorTransform.GetScale3D().Z;

	UCapsuleComponent* capsule = GetCapsuleComponent();
	IKTraceDistance = capsule->GetScaledCapsuleHalfHeight();
}

// Called every frame
void AVRBoundCharacter::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	USkeletalMeshComponent *SkeletalMeshComponent = GetMesh();
	FTransform LocalToWorld = SkeletalMeshComponent->ComponentToWorld;
	FTransform WorldToLocal(SkeletalMeshComponent->ComponentToWorld.ToMatrixWithScale().Inverse());
	FTransform headSocketTransform = HeadSocket->GetSocketTransform(SkeletalMeshComponent); // world space

	// rotate the character
	FVector rightFistPos = DesiredRightFistTransform.GetLocation();
	FVector leftFistPos = DesiredLeftFistTransform.GetLocation();
	FVector rightFistDir = GetXYDir(LocalToWorld.GetLocation() - rightFistPos);
	FVector leftFistDir = GetXYDir(LocalToWorld.GetLocation() - leftFistPos);
	FVector fistsMeanDir = 0.5f * (rightFistDir + leftFistDir);
	fistsMeanDir = fistsMeanDir.GetSafeNormal();
	FVector headForward = headSocketTransform.TransformVector(-FVector::RightVector);
	FVector bodyForward = LocalToWorld.TransformVector(-FVector::RightVector);

	float rotSpeed = FVector::DotProduct(headForward, fistsMeanDir);
	if (rotSpeed < 0.0f) rotSpeed = 0.0f;
	float angle = GetXYAngle(bodyForward, headForward);

	FQuat rotQ;
	rotQ = FQuat::MakeFromEuler(FVector(0.0f, 0.0f, angle) * DeltaTime * rotSpeed);
	AddActorWorldRotation(rotQ);
	//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::SanitizeFloat(rotSpeed));
	//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, bodyForward.ToString());

	// update the head
	float oldHeadYaw = HeadYaw;
	float oldHeadPitch = HeadPitch;
	float oldHeadRoll = HeadRoll;
	if (UAnimInstance *AnimInst = SkeletalMeshComponent->GetAnimInstance())
	{
		UFloatProperty* HeadYawProp = FindField<UFloatProperty>(AnimInst->GetClass(), TEXT("HeadYaw"));
		UFloatProperty* HeadPitchProp = FindField<UFloatProperty>(AnimInst->GetClass(), TEXT("HeadPitch"));
		UFloatProperty* HeadRollProp = FindField<UFloatProperty>(AnimInst->GetClass(), TEXT("HeadRoll"));

		if (HeadYawProp != NULL)	oldHeadYaw = HeadYawProp->GetPropertyValue_InContainer(AnimInst);
		if (HeadPitchProp != NULL)	oldHeadPitch = HeadPitchProp->GetPropertyValue_InContainer(AnimInst);
		if (HeadRollProp != NULL)	oldHeadRoll = HeadRollProp->GetPropertyValue_InContainer(AnimInst);
	}

	FTransform desiredHeadTransformLocalSpace;
	FTransform::Multiply(&desiredHeadTransformLocalSpace, &DesiredHeadTransform, &WorldToLocal);

	float desiredHeadYaw = desiredHeadTransformLocalSpace.Rotator().Yaw;
	float desiredHeadPitch = DesiredHeadTransform.Rotator().Pitch;
	float desiredHeadRoll = DesiredHeadTransform.Rotator().Roll;
	HeadYaw = FMath::FInterpTo(oldHeadYaw, desiredHeadYaw, DeltaTime, InterpSpeed);
	HeadPitch = FMath::FInterpTo(oldHeadPitch, desiredHeadPitch, DeltaTime, InterpSpeed);
	HeadRoll = FMath::FInterpTo(oldHeadRoll, desiredHeadRoll, DeltaTime, InterpSpeed);

	FVector socketPos = headSocketTransform.TransformPosition(FVector::ZeroVector);
	FVector desiredLocation = DesiredHeadTransform.GetLocation();

	// move character so that the socket is in the desired head position
	FVector currentPos = socketPos;
	currentPos.X = GetCapsuleComponent()->GetComponentTransform().GetLocation().X;
	currentPos.Y = GetCapsuleComponent()->GetComponentTransform().GetLocation().Y;
	//currentPos.Z = GetCapsuleComponent()->GetComponentTransform().GetLocation().Z;
	FVector moveDir = desiredLocation - currentPos;
	FVector moveDirXY = moveDir;
	moveDirXY.Z = 0.0f;
	float moveDirL;
	FVector moveDirN;
	moveDirXY.ToDirectionAndLength(moveDirN, moveDirL);

	FVector actorPos = GetActorLocation();
	FVector desiredActorPos = actorPos + moveDirXY;
	FVector newActorPos = FMath::VInterpTo(actorPos, desiredActorPos, DeltaTime, InterpSpeed);
	//SetActorLocation(newActorPos);
	AddMovementInput(moveDirN, moveDirL*DeltaTime);

	// handle the height differently
	FVector pos = SkeletalMeshComponent->GetRelativeTransform().GetLocation();
	pos.Z += moveDir.Z;
	SkeletalMeshComponent->SetRelativeLocation(pos);
	//SkeletalMeshComponent->AddLocalOffset(desiredLocation);

	// Update feet IK
	float rightFootOffset = IKFootTrace(IKTraceDistance, TEXT("RightFootSocket"));
	IKOffsetRightFoot = FMath::FInterpTo(IKOffsetRightFoot, rightFootOffset, DeltaTime, InterpSpeed);

	float leftFootOffset = IKFootTrace(IKTraceDistance, TEXT("LeftFootSocket"));
	IKOffsetLeftFoot = FMath::FInterpTo(IKOffsetLeftFoot, leftFootOffset, DeltaTime, InterpSpeed);
}

void AVRBoundCharacter::SetHeadDesiredTransform(const FTransform HeadTransform)
{
	DesiredHeadTransform = HeadTransform;
}

float AVRBoundCharacter::IKFootTrace(float TraceDistance, FName SocketName)
{
	USkeletalMeshComponent *SkeletalMeshComponent = GetMesh();
	float heightOffset = SkeletalMeshComponentOriginalHeight - SkeletalMeshComponent->GetRelativeTransform().GetLocation().Z;

	const USkeletalMeshSocket* socket = SkeletalMeshComponent->GetSocketByName(SocketName);
	FTransform socketTransform = socket->GetSocketTransform(SkeletalMeshComponent);
	FVector socketPos = socketTransform.TransformPosition(FVector::ZeroVector);

	FVector actorPos = GetActorLocation();

	FVector start = FVector(socketPos.X, socketPos.Y, actorPos.Z);
	FVector end = FVector(socketPos.X, socketPos.Y, actorPos.Z - TraceDistance);

	FHitResult hitRes;
	FCollisionQueryParams qParams;
	if (ActorLineTraceSingle(hitRes, start, end, ECC_Visibility, qParams))
	{
		return heightOffset; // for now

		// there was a hit
		//return heightOffset + (end - hitRes.Location).Size() / TransformZScale;
	}
	else
	{
		return heightOffset;
	}
}

float AVRBoundCharacter::GetIKOffsetRightFoot()
{
	return IKOffsetRightFoot;
}

float AVRBoundCharacter::GetIKOffsetLeftFoot()
{
	return IKOffsetLeftFoot;
}

FVector AVRBoundCharacter::GetDirRightFoot()
{
	USkeletalMeshComponent *SkeletalMeshComponent = GetMesh();
	const USkeletalMeshSocket* socket = SkeletalMeshComponent->GetSocketByName(TEXT("RightFootSocket"));
	FTransform socketTransform = socket->GetSocketTransform(SkeletalMeshComponent);
	FVector socketUp = socketTransform.TransformVector(FVector::UpVector);

	FMatrix LocalToWorldInverse = SkeletalMeshComponent->ComponentToWorld.ToMatrixWithScale().Inverse();
	FVector localDir = LocalToWorldInverse.TransformVector(socketUp);
	localDir = GetXYDir(localDir);
	return localDir;
}

FVector AVRBoundCharacter::GetDirLeftFoot()
{
	USkeletalMeshComponent *SkeletalMeshComponent = GetMesh();
	const USkeletalMeshSocket* socket = SkeletalMeshComponent->GetSocketByName(TEXT("LeftFootSocket"));
	FTransform socketTransform = socket->GetSocketTransform(SkeletalMeshComponent);
	FVector socketUp = socketTransform.TransformVector(FVector::UpVector);

	FMatrix LocalToWorldInverse = SkeletalMeshComponent->ComponentToWorld.ToMatrixWithScale().Inverse();
	FVector localDir = LocalToWorldInverse.TransformVector(socketUp);
	localDir = GetXYDir(localDir);
	return localDir;
}

float AVRBoundCharacter::GetPitchRotationOfHead()
{
	return HeadPitch;
}

float AVRBoundCharacter::GetYawRotationOfHead()
{
	return HeadYaw;
}

float AVRBoundCharacter::GetRollRotationOfHead()
{
	return HeadRoll;
}

void AVRBoundCharacter::SetRightFistDesiredTransform(const FTransform FistTransform)
{
	DesiredRightFistTransform = FistTransform;
}

void AVRBoundCharacter::SetLeftFistDesiredTransform(const FTransform FistTransform)
{
	DesiredLeftFistTransform = FistTransform;
}

FTransform AVRBoundCharacter::GetRightFistDesiredTransform()
{
	return DesiredRightFistTransform;
}

FTransform AVRBoundCharacter::GetLeftFistDesiredTransform()
{
	return DesiredLeftFistTransform;
}

void AVRBoundCharacter::OnConstruction(const FTransform & Transform)
{
	USkeletalMeshComponent *SkeletalMeshComponent = GetMesh();
	if (UAnimInstance *AnimInst = SkeletalMeshComponent->GetAnimInstance())
	{
		UFloatProperty* HeadYawProp = FindField<UFloatProperty>(AnimInst->GetClass(), TEXT("HeadYaw"));
		UFloatProperty* HeadPitchProp = FindField<UFloatProperty>(AnimInst->GetClass(), TEXT("HeadPitch"));
		UFloatProperty* HeadRollProp = FindField<UFloatProperty>(AnimInst->GetClass(), TEXT("HeadRoll"));

		if (HeadYawProp != NULL)	HeadYaw = HeadYawProp->GetPropertyValue_InContainer(AnimInst);
		if (HeadPitchProp != NULL)	HeadPitch = HeadPitchProp->GetPropertyValue_InContainer(AnimInst);
		if (HeadRollProp != NULL)	HeadRoll = HeadRollProp->GetPropertyValue_InContainer(AnimInst);
	}
}

