// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"
#include "TerminalActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Components/WidgetInteractionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

/**
 * Constructor - Sets up the first-person camera and mesh.
 */
APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// ========================================
	// First-Person Camera Setup
	// ========================================
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetMesh(), FName("HeadSocket"));
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 10.0f, 10.0f)); 
	FirstPersonCamera->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	FirstPersonCamera->bUsePawnControlRotation = true; // Camera follows controller rotation

	// ========================================
	// Character Mesh Setup
	// ========================================
	// Position mesh below capsule and rotate to face forward
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
}

/**
 * Makes the player stand up from the terminal.
 * Restores camera control, input mode, and notifies the terminal.
 */
void APlayerCharacter::StandUpFromTerminal()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// Notify the terminal that the player is leaving
	if (ATerminalActor* Term = Cast<ATerminalActor>(LastTerminalUsed))
	{
		Term->OnPlayerExit();
	}

	// ========================================
	// Switch Camera Back to Player
	// ========================================
	// Use smooth blend for comfortable transition
	PC->SetViewTargetWithBlend(this, 1.0f);

	// ========================================
	// Restore Game Input Mode
	// ========================================
	PC->SetInputMode(FInputModeGameOnly());
	PC->bShowMouseCursor = false;

	// ========================================
	// Clear Terminal State
	// ========================================
	bUsingTerminal = false;
	LastTerminalUsed = nullptr;
}

/**
 * Called when the game starts or when spawned.
 */
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

/**
 * Starts trackball scroll mode.
 * Hides cursor and locks input for smooth grid scrolling.
 */
void APlayerCharacter::StartScrolling()
{
	if (!bUsingTerminal)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Hide cursor for trackball-style scrolling
		PC->bShowMouseCursor = false;

		// Lock to game-only input (mouse movement controls scroll, not cursor)
		PC->SetInputMode(FInputModeGameOnly());
	}
}

/**
 * Stops trackball scroll mode.
 * Shows cursor and allows UI interaction again.
 */
void APlayerCharacter::StopScrolling()
{
	if (!bUsingTerminal)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Show cursor for clicking UI buttons
		PC->bShowMouseCursor = true;

		// Allow both game input and UI interaction
		PC->SetInputMode(FInputModeGameAndUI());
	}
}

/**
 * Called every frame.
 */
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/**
 * Sets up all input bindings for movement, camera, and terminal interaction.
 */
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// ========================================
	// Movement Bindings
	// ========================================
	PlayerInputComponent->BindAxis("MoveForward", this, &APlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APlayerCharacter::MoveRight);
	
	// ========================================
	// Camera Look Bindings
	// ========================================
	PlayerInputComponent->BindAxis("Turn", this, &APlayerCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &APlayerCharacter::LookUp);
	
	// ========================================
	// Interaction Bindings
	// ========================================
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &APlayerCharacter::Interact);
	PlayerInputComponent->BindAction("TerminalInteract", IE_Pressed, this, &APlayerCharacter::Interact);

	// ========================================
	// Trackball Scroll Bindings
	// ========================================
	// NOTE: Make sure these axis names match your Project Settings -> Input exactly!
	
	// Scroll modifier key (hold to enable trackball scrolling)
	PlayerInputComponent->BindAction("ScrollModifier", IE_Pressed, this, &APlayerCharacter::StartScrolling);
	PlayerInputComponent->BindAction("ScrollModifier", IE_Released, this, &APlayerCharacter::StopScrolling);
	
	// Scroll axes (mouse movement when ScrollModifier is held)
	PlayerInputComponent->BindAxis("TerminalScroll_X", this, &APlayerCharacter::InputScrollX);
	PlayerInputComponent->BindAxis("TerminalScroll_Y", this, &APlayerCharacter::InputScrollY);
}

// ========================================
// MOVEMENT INPUT HANDLERS
// ========================================

/**
 * Handles forward/backward movement input (W/S keys).
 * Disabled when using terminal.
 */
void APlayerCharacter::MoveForward(float AxisValue)
{
	// Don't allow walking while seated at terminal
	if (!bUsingTerminal)
	{
		AddMovementInput(GetActorForwardVector(), AxisValue);
	}
}

/**
 * Handles left/right movement input (A/D keys).
 * Disabled when using terminal.
 */
void APlayerCharacter::MoveRight(float AxisValue)
{
	if (!bUsingTerminal)
	{
		AddMovementInput(GetActorRightVector(), AxisValue);
	}
}

/**
 * Handles vertical camera rotation (mouse Y).
 * Disabled when using terminal.
 */
void APlayerCharacter::LookUp(float Rate)
{
	if (!bUsingTerminal)
	{
		AddControllerPitchInput(Rate);
	}
}

/**
 * Handles horizontal camera rotation (mouse X).
 * Disabled when using terminal.
 */
void APlayerCharacter::Turn(float Rate)
{
	if (!bUsingTerminal)
	{
		AddControllerYawInput(Rate);
	}
}

// ========================================
// TRACKBALL SCROLLING (Terminal Mode)
// ========================================

/**
 * Handles horizontal trackball scroll input.
 * Sends mouse X movement to the terminal for grid scrolling.
 */
void APlayerCharacter::InputScrollX(float AxisValue)
{
	// Only process if using terminal and there's actual input
	if (bUsingTerminal && LastTerminalUsed && AxisValue != 0.f)
	{
		if (ATerminalActor* Term = Cast<ATerminalActor>(LastTerminalUsed))
		{
			// ========================================
			// Sensitivity Multiplier
			// ========================================
			// Reduce to 0.1 (or even 0.05) to slow down scrolling
			// Higher values = faster, more sensitive scrolling
			// Lower values = slower, more precise scrolling
			float Sensitivity = 0.1f;
			
			// Send X input to terminal (Y component is 0)
			Term->ApplyTrackballInput(AxisValue * Sensitivity, 0.f);
		}
	}
}

/**
 * Handles vertical trackball scroll input.
 * Sends mouse Y movement to the terminal for grid scrolling.
 */
void APlayerCharacter::InputScrollY(float AxisValue)
{
	if (bUsingTerminal && LastTerminalUsed && AxisValue != 0.f)
	{
		if (ATerminalActor* Term = Cast<ATerminalActor>(LastTerminalUsed))
		{
			// Same sensitivity as X axis for consistent feel
			float Sensitivity = 0.1f;
			
			// Send Y input to terminal (X component is 0)
			Term->ApplyTrackballInput(0.f, AxisValue * Sensitivity);
		}
	}
}

// ========================================
// TERMINAL INTERACTION
// ========================================

/**
 * Handles the interact key press.
 * When standing: Attempts to sit at a nearby terminal
 * When seated: Stands up from the current terminal
 */
void APlayerCharacter::Interact()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !FirstPersonCamera)
	{
		return;
	}

	// ========================================
	// Case 1: Already Using Terminal - Stand Up
	// ========================================
	if (bUsingTerminal)
	{
		StandUpFromTerminal();
		return;
	}

	// ========================================
	// Case 2: Try to Sit Down at Terminal
	// ========================================
	
	// Perform raycast from camera to find terminals
	FHitResult Hit;
	FVector Start = FirstPersonCamera->GetComponentLocation();
	FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractRange;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this); // Don't hit ourselves

	// Raycast on the custom interaction channel (ECC_GameTraceChannel1)
	// Make sure your terminals are set to block this channel!
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_GameTraceChannel1, Params))
	{
		ATerminalActor* HitTerminal = Cast<ATerminalActor>(Hit.GetActor());
		
		// ========================================
		// Power Check (Optional)
		// ========================================
		// TODO: Check if HitTerminal->IsPoweredOn() before sitting
		// This prevents interacting with unpowered terminals
		
		if (HitTerminal)
		{
			// Store reference to this terminal
			LastTerminalUsed = HitTerminal;

			// ========================================
			// Switch Camera to Terminal View
			// ========================================
			// Smooth 1 second blend to terminal camera
			PC->SetViewTargetWithBlend(HitTerminal, 1.0f);
			
			// ========================================
			// Set Input Mode for Terminal Use
			// ========================================
			// GameAndUI allows both mouse cursor and game input
			PC->SetInputMode(FInputModeGameAndUI());
			PC->bShowMouseCursor = true; // Show cursor for clicking UI

			// ========================================
			// Notify Terminal of Player Interaction
			// ========================================
			HitTerminal->OnPlayerInteract(); 

			// Update state
			bUsingTerminal = true;
		}
	}
}