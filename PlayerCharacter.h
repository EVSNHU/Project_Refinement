// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "PlayerCharacter.generated.h"

UCLASS()
class PROJECT_REFINEMENT_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StandUpFromTerminal();

protected:
	virtual void BeginPlay() override;

private:
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	void LookUp(float Rate);
	void Turn(float Rate);
	void Interact();

	// --- NEW: Trackball Inputs ---
	void InputScrollX(float AxisValue);
	void InputScrollY(float AxisValue);

	UPROPERTY(EditAnywhere)
	float LookSensistivity = 45.f;

	UPROPERTY(EditAnywhere)
	float InteractRange = 300.f;

	

	void StartScrolling();
	void StopScrolling();

public: 
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    	UCameraComponent* FirstPersonCamera;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	UWidgetInteractionComponent* WidgetInteractor;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> TerminalWidgetClass;

	UPROPERTY()
	UUserWidget* TerminalWidget;

	UPROPERTY()
	bool bUsingTerminal = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AActor* LastTerminalUsed = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UCameraShakeBase> ShakeCameraClass;
};