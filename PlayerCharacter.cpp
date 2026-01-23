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

APlayerCharacter::APlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetMesh(), FName("HeadSocket"));
    FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 10.0f, 10.0f)); 
    FirstPersonCamera->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
    FirstPersonCamera->bUsePawnControlRotation = true;

    GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
    GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
}

void APlayerCharacter::StandUpFromTerminal()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    if (ATerminalActor* Term = Cast<ATerminalActor>(LastTerminalUsed))
    {
        Term->OnPlayerExit();
    }

    PC->SetViewTargetWithBlend(this, 1.0f);

    PC->SetInputMode(FInputModeGameOnly());
    PC->bShowMouseCursor = false;

    bUsingTerminal = false;
    LastTerminalUsed = nullptr;
    
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void APlayerCharacter::StartScrolling()
{
    if (!bUsingTerminal) return;

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = false;

        PC->SetInputMode(FInputModeGameOnly());
    }
}

void APlayerCharacter::StopScrolling()
{
    if (!bUsingTerminal) return;
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;

        PC->SetInputMode(FInputModeGameAndUI());
    }
}

void APlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Movement
    PlayerInputComponent->BindAxis("MoveForward",this, &APlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &APlayerCharacter::MoveRight);
    
    // Camera Look
    PlayerInputComponent->BindAxis("Turn", this, &APlayerCharacter::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &APlayerCharacter::LookUp);
    
    // Interaction
    PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &APlayerCharacter::Interact);
    PlayerInputComponent->BindAction("TerminalInteract", IE_Pressed, this, &APlayerCharacter::Interact);

    // --- NEW: Bind Trackball Inputs ---
    // Make sure these names match Project Settings -> Input -> Axis Mappings exactly!
    PlayerInputComponent->BindAction("ScrollModifier", IE_Pressed, this, &APlayerCharacter::StartScrolling);
    PlayerInputComponent->BindAction("ScrollModifier", IE_Released, this, &APlayerCharacter::StopScrolling);
    PlayerInputComponent->BindAxis("TerminalScroll_X", this, &APlayerCharacter::InputScrollX);
    PlayerInputComponent->BindAxis("TerminalScroll_Y", this, &APlayerCharacter::InputScrollY);
}

void APlayerCharacter::MoveForward(float AxisValue)
{
    if (!bUsingTerminal) // Optional: Stop moving feet while sitting
    {
        AddMovementInput(GetActorForwardVector(), AxisValue);
    }
}

void APlayerCharacter::MoveRight(float AxisValue)
{
    if (!bUsingTerminal)
    {
        AddMovementInput(GetActorRightVector(), AxisValue);
    }
}

void APlayerCharacter::LookUp(float Rate)
{
    if (!bUsingTerminal)
    {
        AddControllerYawInput(Rate);
    }
}

void APlayerCharacter::Turn(float Rate)
{
    if (!bUsingTerminal)
    {
        AddControllerPitchInput(Rate);
    }
}

// --- NEW: Trackball Implementation ---

void APlayerCharacter::InputScrollX(float AxisValue)
{
    // Check if we are using the terminal
    if (bUsingTerminal && LastTerminalUsed && AxisValue != 0.f)
    {
        if (ATerminalActor* Term = Cast<ATerminalActor>(LastTerminalUsed))
        {
            // MULTIPLY BY 0.1f (Or even 0.05f) TO SLOW IT DOWN
            float Sensitivity = 0.1f; 
            Term->ApplyTrackballInput(AxisValue * Sensitivity, 0.f);
        }
    }
}

void APlayerCharacter::InputScrollY(float AxisValue)
{
    if (bUsingTerminal && LastTerminalUsed && AxisValue != 0.f)
    {
        if (ATerminalActor* Term = Cast<ATerminalActor>(LastTerminalUsed))
        {
            // Same sensitivity here
            float Sensitivity = 0.1f;
            Term->ApplyTrackballInput(0.f, AxisValue * Sensitivity);
        }
    }
}

void APlayerCharacter::Interact()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !FirstPersonCamera) return;

    // 1. IF ALREADY SITTING: Just stand up
    if (bUsingTerminal)
    {
        StandUpFromTerminal();
        return;
    }

    // 2. IF STANDING: Try to sit down (Raycast)
    FHitResult Hit;
    FVector Start = FirstPersonCamera->GetComponentLocation();
    FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractRange;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_GameTraceChannel1, Params))
    {
        ATerminalActor* HitTerminal = Cast<ATerminalActor>(Hit.GetActor());
        
        // --- POWER CHECK HERE ---
        // Ideally, check HitTerminal->IsPoweredOn() here before sitting down
        if (HitTerminal)
        {
            LastTerminalUsed = HitTerminal;

            PC->SetViewTargetWithBlend(HitTerminal, 1.0f);
            
            // Set to GameAndUI so we can click buttons, 
            // but keep Mouse Cursor visible initially.
            PC->SetInputMode(FInputModeGameAndUI());
            PC->bShowMouseCursor = true; 

            HitTerminal->OnPlayerInteract(); 

            bUsingTerminal = true;
        }
    }
}

