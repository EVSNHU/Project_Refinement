// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "PlayerCharacter.generated.h"

/**
 * Player Character for the Terminal Refinement game.
 * 
 * Handles:
 * - First-person camera and movement
 * - Terminal interaction (sitting down / standing up)
 * - Camera switching between player view and terminal view
 * - Trackball-style scrolling when using the terminal
 * - Input mode switching (game vs UI)
 * 
 * Gameplay States:
 * - Walking: Normal first-person controls, can look around and interact
 * - Using Terminal: Camera locked to terminal, mouse controls UI and trackball scrolling
 */
UCLASS()
class PROJECT_REFINEMENT_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	/**
	 * Makes the player stand up from the terminal.
	 * Switches camera back to player, restores movement controls.
	 * Called when player exits terminal or presses interact while seated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StandUpFromTerminal();

protected:
	virtual void BeginPlay() override;

public: 
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ========================================
	// Camera
	// ========================================
	
	/**
	 * First-person camera attached to the character's head socket.
	 * This is the player's main view when walking around.
	 */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	UCameraComponent* FirstPersonCamera;

	// ========================================
	// UI & Interaction
	// ========================================
	
	/**
	 * Widget interaction component for clicking UI elements.
	 * Allows the player to interact with 3D widgets in the world.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	UWidgetInteractionComponent* WidgetInteractor;
	
	/**
	 * The widget class to spawn when using a terminal.
	 * Set this in Blueprint to your terminal UI widget.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> TerminalWidgetClass;

	/**
	 * Active instance of the terminal widget.
	 * Spawned when sitting down, destroyed when standing up.
	 */
	UPROPERTY()
	UUserWidget* TerminalWidget;

	// ========================================
	// Terminal State
	// ========================================
	
	/**
	 * Whether the player is currently using a terminal.
	 * When true, movement is disabled and camera is locked to terminal view.
	 */
	UPROPERTY()
	bool bUsingTerminal = false;

	/**
	 * Reference to the terminal the player is currently using.
	 * Needed to send input and call exit functions.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AActor* LastTerminalUsed = nullptr;

	// ========================================
	// Camera Effects
	// ========================================
	
	/**
	 * Camera shake class for stress/proximity effects.
	 * Triggered when scary numbers are nearby.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UCameraShakeBase> ShakeCameraClass;

private:
	// ========================================
	// Movement Input Handlers
	// ========================================
	
	/**
	 * Handles forward/backward movement input.
	 * Disabled when using terminal.
	 */
	void MoveForward(float AxisValue);
	
	/**
	 * Handles left/right movement input.
	 * Disabled when using terminal.
	 */
	void MoveRight(float AxisValue);
	
	/**
	 * Handles vertical camera look input.
	 * Disabled when using terminal.
	 */
	void LookUp(float Rate);
	
	/**
	 * Handles horizontal camera look input.
	 * Disabled when using terminal.
	 */
	void Turn(float Rate);

	// ========================================
	// Interaction
	// ========================================
	
	/**
	 * Handles the interact key press.
	 * When standing: Raycasts to find and sit at a terminal
	 * When seated: Stands up from the terminal
	 */
	void Interact();

	// ========================================
	// Trackball Scrolling (Terminal Mode)
	// ========================================
	
	/**
	 * Handles horizontal trackball scroll input.
	 * Only active when using a terminal.
	 */
	void InputScrollX(float AxisValue);
	
	/**
	 * Handles vertical trackball scroll input.
	 * Only active when using a terminal.
	 */
	void InputScrollY(float AxisValue);

	/**
	 * Starts trackball scroll mode.
	 * Hides mouse cursor, locks input to game mode for smooth scrolling.
	 * Called when scroll modifier key is pressed.
	 */
	void StartScrolling();
	
	/**
	 * Stops trackball scroll mode.
	 * Shows mouse cursor, allows UI interaction again.
	 * Called when scroll modifier key is released.
	 */
	void StopScrolling();

	// ========================================
	// Settings
	// ========================================
	
	/** Camera look sensitivity multiplier */
	UPROPERTY(EditAnywhere)
	float LookSensistivity = 45.f;

	/** Maximum distance for terminal interaction raycast */
	UPROPERTY(EditAnywhere)
	float InteractRange = 300.f;
};