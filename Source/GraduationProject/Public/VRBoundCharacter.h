// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Character.h"
#include "VRBoundCharacter.generated.h"

UCLASS()
class GRADUATIONPROJECT_API AVRBoundCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRBoundCharacter();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	//***************************************************
	//						HEAD
	//***************************************************

	// Sets the desired transformation for the head part of the character.
	UFUNCTION(BlueprintCallable, Category = "IK", meta = (DisplayName = "Set Head Desired Transform", Keywords = "set head desired transformation"))
		void SetHeadDesiredTransform(const FTransform HeadTransform);

	// Sets the desired transformation for the head part of the character.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IK", meta = (DisplayName = "IK Foot Trace", Keywords = "inverse kinematics ik foot trace"))
		float IKFootTrace(float TraceDistance, FName SocketName);

	// Gets the pitch rotation of the head
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FK", meta = (DisplayName = "Get The Pitch Rotation Of The Head", Keywords = "get the pitch rotation of the head"))
		float GetPitchRotationOfHead();

	// Gets the yaw rotation of the head
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FK", meta = (DisplayName = "Get The Yaw Rotation Of The Head", Keywords = "get the yaw rotation of the head"))
		float GetYawRotationOfHead();

	// Gets the roll rotation of the head
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FK", meta = (DisplayName = "Get The Roll Rotation Of The Head", Keywords = "get the roll rotation of the head"))
		float GetRollRotationOfHead();

	//***************************************************
	//						FEET
	//***************************************************

	// Gets the IK offset of the right foot
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IK", meta = (DisplayName = "Get IK Offset Right Foot", Keywords = "get inverse kinematics ik right foot offset"))
		float GetIKOffsetRightFoot();

	// Gets the IK offset of the left foot
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IK", meta = (DisplayName = "Get IK Offset Left Foot", Keywords = "get inverse kinematics ik left foot offset"))
		float GetIKOffsetLeftFoot();

	// Gets the feet direction of the right foot in component space
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FK", meta = (DisplayName = "Get Dir Right Foot", Keywords = "get feet direction right foot component space"))
		FVector GetDirRightFoot();

	// Gets the feet direction of the left foot in component space
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FK", meta = (DisplayName = "Get Dir Left Foot", Keywords = "get feet direction left foot component space"))
		FVector GetDirLeftFoot();

	//***************************************************
	//						HANDS
	//***************************************************

	// Sets the desired transformation for the right fist.
	UFUNCTION(BlueprintCallable, Category = "IK", meta = (DisplayName = "Set Right Fist Desired Transform", Keywords = "set right fist desired transformation"))
		void SetRightFistDesiredTransform(const FTransform FistTransform);

	// Sets the desired transformation for the left fist.
	UFUNCTION(BlueprintCallable, Category = "IK", meta = (DisplayName = "Set Left Fist Desired Transform", Keywords = "set left fist desired transformation"))
		void SetLeftFistDesiredTransform(const FTransform FistTransform);

	// Gets the yaw rotation of the head
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FK", meta = (DisplayName = "Get Right Fist Desired Transform", Keywords = "get right fist desired transform"))
		FTransform GetRightFistDesiredTransform();

	// Gets the roll rotation of the head
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FK", meta = (DisplayName = "Get Left Fist Desired Transform", Keywords = "get left fist desired transform"))
		FTransform GetLeftFistDesiredTransform();


	// Speed of the interpolation of the feet offset.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
		float InterpSpeed;

private:
	// On construction.
	virtual void OnConstruction(const FTransform& Transform);

	FTransform DesiredHeadTransform;
	const USkeletalMeshSocket* HeadSocket;
	float HeadPitch;
	float HeadYaw;
	float HeadRoll;	
	
	FTransform DesiredRightFistTransform;
	FTransform DesiredLeftFistTransform;

	float SkeletalMeshComponentOriginalHeight;
	float TransformZScale;
	float IKTraceDistance;
	float IKOffsetRightFoot;
	float IKOffsetLeftFoot;
};
