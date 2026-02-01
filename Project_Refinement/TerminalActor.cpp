#include "TerminalActor.h"
#include "AudioMixerBlueprintLibrary.h"
#include "PlayerCharacter.h"
#include "Math/UnrealMathUtility.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Algo/RandomShuffle.h"
#include "TimerManager.h"

/**
 * Constructor - Initializes all components and default values.
 */
ATerminalActor::ATerminalActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// ========================================
	// Component Setup
	// ========================================
	CRTMonitor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CRTMonitor"));
	RootComponent = CRTMonitor;

	TerminalCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TerminalCamera"));
	TerminalCamera->SetupAttachment(RootComponent);
	TerminalCamera->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	TerminalCamera->SetRelativeRotation(FRotator(-10.f, 180.f, 0.f));

	// ========================================
	// Array Initialization
	// ========================================
	// Initialize arrays with safe default sizes
	// (Will be properly resized in GenerateGrid)
	const int32 InitialSize = GridWidth * GridHeight;
	GridNumbers.Init(0, InitialSize);
	ProgressBars.Init(0.f, 4);
	HighlightedPrimes.Init(false, InitialSize);
	ScaryActive.Init(false, InitialSize);
	
	// Cooldown system initialization
	bBarCoolingDown.Init(false, 4);
	BarCooldownRemaining.Init(0.f, 4);

	// ========================================
	// Default Settings
	// ========================================
	MaxSensorDistance = 15.0f;
	ScrollX = 500;  // Start in middle of 1000x1000 grid
	ScrollY = 500;
}

/**
 * Called when the game starts or when spawned.
 */
void ATerminalActor::BeginPlay()
{
	Super::BeginPlay();
	GenerateGrid();
}

// ========================================
// GRID GENERATION & PRIME LOGIC
// ========================================

/**
 * Generates the complete global grid (1000x1000) with random numbers.
 * Also spawns "scary" numbers in a sector pattern (one per 50x50 block).
 */
void ATerminalActor::GenerateGrid()
{
	UE_LOG(LogTemp, Warning, TEXT("!!! GenerateGrid is STARTING !!!"));
	
	// Calculate total size of the global grid
	const int32 TotalGlobalCount = GlobalMapWidth * GlobalMapHeight;
	
	// Initialize arrays to correct size
	GlobalGridNumbers.Init(0, TotalGlobalCount);
	ScaryActive.Init(false, TotalGlobalCount);

	// ========================================
	// Step 1: Fill Grid with Random Numbers (1-9)
	// ========================================
	for (int32 i = 0; i < TotalGlobalCount; ++i)
	{
		GlobalGridNumbers[i] = FMath::RandRange(1, 9);
	}

	// ========================================
	// Step 2: Spawn Scary Numbers (Sector Pattern)
	// ========================================
	// Divide the grid into 50x50 sectors and spawn one scary number per sector
	// This ensures even distribution across the infinite grid
	int32 SectorSize = 50; 
	
	for (int32 y = 0; y < GlobalMapHeight; y += SectorSize)
	{
		for (int32 x = 0; x < GlobalMapWidth; x += SectorSize)
		{
			// Pick a random tile within this 50x50 block
			// Offset by 5 to avoid edges
			int32 RandX = x + FMath::RandRange(5, SectorSize - 5);
			int32 RandY = y + FMath::RandRange(5, SectorSize - 5);
			int32 GlobalIdx = RandY * GlobalMapWidth + RandX;

			// Mark this tile as scary
			if (ScaryActive.IsValidIndex(GlobalIdx))
			{
				ScaryActive[GlobalIdx] = true;
			}
			
			UE_LOG(LogTemp, Warning, TEXT("Scary Number spawned at Global Index: %d"), GlobalIdx);
		}
	}
	
	// Update the visible grid window
	OnGridScrolled();
}

/**
 * Checks if a number is prime (only 2, 3, 5, 7 in our game).
 * Used for special visual effects or mechanics.
 */
bool ATerminalActor::IsPrime(const int32 Number) const
{
	return (Number == 2 || Number == 3 || Number == 5 || Number == 7);
}

/**
 * Checks if a screen-space tile index is currently scary.
 * Converts screen index to global index and checks ScaryActive array.
 */
bool ATerminalActor::IsIndexScary(int32 ScreenIndex) const
{
	// Convert from visible grid (0-99) to global grid (0-999,999)
	const int32 GlobalIdx = GetGlobalIndexFromScreenIndex(ScreenIndex);

	if (ScaryActive.IsValidIndex(GlobalIdx))
	{
		return ScaryActive[GlobalIdx];
	}
	return false;
}

/**
 * Randomly activates one non-scary prime number as scary.
 * Used to increase difficulty/tension during gameplay.
 */
void ATerminalActor::HighlightRandomPrime()
{
	if (PrimeIndices.Num() == 0)
	{
		return;
	}

	// Build list of prime numbers that aren't already scary
	TArray<int32> Candidates;
	for (int32 Idx : PrimeIndices)
	{
		if (ScaryActive.IsValidIndex(Idx) && !ScaryActive[Idx])
		{
			Candidates.Add(Idx);
		}
	}

	// If no candidates, all primes are already scary
	if (Candidates.Num() == 0)
	{
		return;
	}

	// Pick one randomly and make it scary
	const int32 Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	ScaryActive[Chosen] = true;
}

// ========================================
// PROGRESS BAR MANAGEMENT
// ========================================

/**
 * Resets all four progress bars to 0% and notifies the UI.
 * Called when starting a new file refinement.
 */
void ATerminalActor::ResetProgressBars()
{
	ProgressBars.Init(0.f, 4);

	// Broadcast update for each bar
	for (int32 BarIndex = 0; BarIndex < 4; ++BarIndex)
	{
		OnProgressUpdated(BarIndex, 0.f);
	}
}

/**
 * Checks if a progress bar can accept new input.
 * A bar is unavailable if it's on cooldown OR already full.
 */
bool ATerminalActor::IsBarAvailable(int32 BarIndex) const
{
	// Validate array bounds
	if (!bBarCoolingDown.IsValidIndex(BarIndex) || !ProgressBars.IsValidIndex(BarIndex))
	{
		return false;
	}

	// Check if bar is on cooldown
	bool bIsCooling = bBarCoolingDown[BarIndex];

	// Check if bar is already full
	bool bIsFull = ProgressBars[BarIndex] >= 1.0f;

	// Bar is available only if it's NOT cooling AND NOT full
	return !bIsCooling && !bIsFull;
}

/**
 * Gets the cooldown progress ratio for a bar.
 * Returns 1.0 when cooldown just started, 0.0 when cooldown is done.
 * Use this to display cooldown timer UI.
 */
float ATerminalActor::GetBarCooldownRatio(int32 BarIndex) const
{
	if (!BarCooldownRemaining.IsValidIndex(BarIndex) || BarCooldownSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(BarCooldownRemaining[BarIndex] / BarCooldownSeconds, 0.f, 1.f);
}

/**
 * Applies the pending chunk value to a specific progress bar.
 * Clamps bar to maximum of 1.0, triggers file completion if appropriate.
 */
void ATerminalActor::ApplChunkToBar(int32 BarIndex)
{
	// Early exit conditions
	if (!bDayActive || PendingChunkValue <= 0.f)
	{
		return;
	}

	if (!ProgressBars.IsValidIndex(BarIndex) || ProgressBars[BarIndex] >= 1.f)
	{
		return;
	}

	// Apply chunk value with 1.5x multiplier
	// (This makes the game feel more rewarding/faster-paced)
	ProgressBars[BarIndex] = FMath::Clamp(
		ProgressBars[BarIndex] + PendingChunkValue * 1.5f,
		0.f,
		1.f
	);

	// Clear the pending chunk
	PendingChunkValue = 0.f;
	
	// Notify systems that chunk was consumed
	OnChunkConsumed();
	OnProgressUpdated(BarIndex, ProgressBars[BarIndex]);

	// Check if file is complete (master progress = 100%)
	if (GetMasterProgress() >= 1.0f)
	{
		OnFileWorkComplete(); 
	}

	// Check if all bars are full (triggers special state)
	if (AllBarsFull())
	{
		OnAllBarsFull();
	}
}

/**
 * Gets the proximity value to the nearest scary number.
 * Returns 0.0 when far away or no scary numbers nearby.
 * Returns 1.0 when a scary number is very close.
 * 
 * This is used to drive stress/anxiety UI elements:
 * - Screen distortion
 * - Sound effects
 * - Camera shake
 * - Warning indicators
 */
float ATerminalActor::GetSensorProximityValue() const
{
	// Detection radius in tiles
	float RealMaxDistance = 15.0f;
	
	float MinDistSq = FMath::Square(RealMaxDistance);
	bool bFound = false;

	// Calculate center of visible screen (accounting for scroll and sub-pixel offset)
	float CenterX = (float)ScrollX + AccumulatorX + 4.5f; // 4.5 = center of 10x10 grid
	float CenterY = (float)ScrollY + AccumulatorY + 4.5f;

	// Search a wide area around the visible screen
	// Need to search beyond the visible grid to detect approaching threats
	int32 SearchRadius = 16; 

	// Scan for scary numbers in the search area
	for (int32 y = ScrollY - SearchRadius; y <= ScrollY + SearchRadius + 10; ++y)
	{
		for (int32 x = ScrollX - SearchRadius; x <= ScrollX + SearchRadius + 10; ++x)
		{
			// ========================================
			// Handle Grid Wrapping (Pac-Man Effect)
			// ========================================
			// The grid wraps at the edges, so we need to use modulo
			
			int32 WrappedX = x % GlobalMapWidth;
			if (WrappedX < 0) WrappedX += GlobalMapWidth;

			int32 WrappedY = y % GlobalMapHeight;
			if (WrappedY < 0) WrappedY += GlobalMapHeight;

			int32 GlobalIdx = WrappedY * GlobalMapWidth + WrappedX;

			// Check if this tile is scary
			if (ScaryActive.IsValidIndex(GlobalIdx) && ScaryActive[GlobalIdx])
			{
				// Calculate distance using raw coordinates (not wrapped)
				// This ensures correct distance calculation across wrap boundaries
				float DistSq = FMath::Square(x - CenterX) + FMath::Square(y - CenterY);
              
				// Track the closest scary number
				if (DistSq < MinDistSq)
				{
					MinDistSq = DistSq;
					bFound = true;
				}
			}
		}
	}

	// If no scary numbers in range, return 0
	if (!bFound)
	{
		return 0.0f;
	}

	// Convert squared distance to regular distance
	float Distance = FMath::Sqrt(MinDistSq);
    
	// Convert distance to 0.0-1.0 range (1.0 = very close, 0.0 = far)
	return FMath::Clamp(1.0f - (Distance / RealMaxDistance), 0.0f, 1.0f);
}

/**
 * Checks if all four progress bars have reached 100%.
 */
bool ATerminalActor::AllBarsFull() const
{
	for (float Bar : ProgressBars)
	{
		if (Bar < 1.f)
		{
			return false;
		}
	}
	return true;
}

/**
 * Virtual function called when all bars are full.
 * Can be overridden in Blueprint for custom behavior.
 */
void ATerminalActor::OnAllBarsFull()
{
	UE_LOG(LogTemp, Warning, TEXT("All bars are full!"));
}

/**
 * Exits terminal mode and returns camera control to the player character.
 */
void ATerminalActor::ExitTerminalMode()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();

	if (PC)
	{
		if (APlayerCharacter* Player = Cast<APlayerCharacter>(PC->GetPawn()))
		{
			Player->StandUpFromTerminal();
		}
	}
}

/**
 * Calculates the master progress (average of all four bars).
 * When this reaches 1.0, the current file is complete.
 */
float ATerminalActor::GetMasterProgress() const
{
	float Total = 0.0;
	for (float BinProgress : ProgressBars)
	{
		Total += BinProgress;
	}

	return FMath::Clamp(Total / 4.0f, 0.0f, 1.0f);
}

// ========================================
// DAY / FILE MANAGEMENT
// ========================================

/**
 * Starts a new workday.
 * Resets counters, generates fresh grid, broadcasts start event.
 */
void ATerminalActor::StartDay()
{
	bDayActive = true;
	DayStartTime = GetWorld()->GetTimeSeconds();
	
	GenerateGrid();
	OnDayStarted();
}

/**
 * Ends the current workday.
 * Sets active flag to false, broadcasts completion event.
 */
void ATerminalActor::EndDay()
{
	bDayActive = false;
	OnDayCompleted();
}

/**
 * Called when a single file reaches 100% completion.
 * Updates counter, checks if all files for the day are done.
 */
void ATerminalActor::OnFileWorkComplete()
{
	FilesRefinedCount++;

	// Broadcast update so UI knows progress (e.g., "1/2 files complete")
	OnFileCompleted(FilesRefinedCount, FilesPerDay);

	// Check if all files for the day are complete
	if (FilesRefinedCount >= FilesPerDay)
	{
		// Day is complete!
		bDayActive = false;
		
		// Calculate how long the day took
		float Duration = GetWorld()->GetTimeSeconds() - DayStartTime;

		// Trigger day complete event
		BP_OnDayComplete(Duration);
	}
	else
	{
		// More files to go - reset and show file selection
		ResetProgressBars();
		BP_OnShowFileSelection();
	}
}

// ========================================
// INFINITE SCROLLING SYSTEM
// ========================================

/**
 * Applies mouse/trackball input to scroll the grid.
 * Uses sub-pixel accumulation for smooth movement.
 * Implements wrapping to create infinite grid illusion.
 */
void ATerminalActor::ApplyTrackballInput(float AxisX, float AxisY)
{
	// ========================================
	// Step 1: Accumulate Movement
	// ========================================
	// Multiply by -1 to invert controls (feel more natural for trackball)
	AccumulatorX += (AxisX * -1.0f) * ScrollSensitivity;
	AccumulatorY += (AxisY * -1.0f) * ScrollSensitivity;

	// ========================================
	// Step 2: Update Integer Scroll Positions
	// ========================================
	// Extract integer part of accumulated movement
	ScrollX += (int32)AccumulatorX;
	ScrollY += (int32)AccumulatorY;

	// ========================================
	// Step 3: Clear Integer Part from Accumulator
	// ========================================
	// Keep only the decimal part for smooth sub-pixel scrolling
	AccumulatorX -= (int32)AccumulatorX;
	AccumulatorY -= (int32)AccumulatorY;

	// ========================================
	// Step 4: Wrap Scroll Positions
	// ========================================
	// This creates the "Pac-Man" effect - grid wraps seamlessly
	
	// Wrap X coordinate
	ScrollX = ScrollX % GlobalMapWidth;
	if (ScrollX < 0) ScrollX += GlobalMapWidth;

	// Wrap Y coordinate
	ScrollY = ScrollY % GlobalMapHeight;
	if (ScrollY < 0) ScrollY += GlobalMapHeight;

	// ========================================
	// Step 5: Update Visual Grid
	// ========================================
	OnGridScrolled();
}

// ========================================
// INDEX CONVERSION HELPERS
// ========================================

/**
 * Converts a screen-space index (0-99) to a global grid index (0-999,999).
 * Handles wrapping for the infinite grid effect.
 */
int32 ATerminalActor::GetGlobalIndexFromScreenIndex(int32 ScreenIndex) const
{
	// ========================================
	// Step 1: Calculate Local Screen Coordinates
	// ========================================
	// Convert 1D index to 2D coordinates in the 10x10 visible grid
	int32 ScreenX = ScreenIndex % GridWidth;
	int32 ScreenY = ScreenIndex / GridWidth;

	// ========================================
	// Step 2: Calculate "Raw" Global Coordinates
	// ========================================
	// Add screen offset to current scroll position
	int32 RawGlobalX = ScrollX + ScreenX;
	int32 RawGlobalY = ScrollY + ScreenY;

	// ========================================
	// Step 3: Wrap X Coordinate (Handle Negatives)
	// ========================================
	// This creates the "Pac-Man" effect horizontally
	int32 WrappedX = RawGlobalX % GlobalMapWidth;
	if (WrappedX < 0) WrappedX += GlobalMapWidth;

	// ========================================
	// Step 4: Wrap Y Coordinate
	// ========================================
	// This creates the "Pac-Man" effect vertically
	int32 WrappedY = RawGlobalY % GlobalMapHeight;
	if (WrappedY < 0) WrappedY += GlobalMapHeight;

	// ========================================
	// Step 5: Convert Back to 1D Index
	// ========================================
	return WrappedY * GlobalMapWidth + WrappedX;
}

/**
 * Gets the number at a specific grid coordinate.
 * Handles wrapping for any coordinate value (even negative or > grid size).
 */
int32 ATerminalActor::GetGridNumber(int32 GridX, int32 GridY) const
{
	// Wrap X coordinate (handle large positive numbers AND negatives)
	int32 WrappedX = GridX % GlobalMapWidth;
	if (WrappedX < 0) WrappedX += GlobalMapWidth;

	// Wrap Y coordinate
	int32 WrappedY = GridY % GlobalMapHeight;
	if (WrappedY < 0) WrappedY += GlobalMapHeight;

	// Calculate index using the safe, wrapped coordinates
	int32 GlobalIdx = WrappedY * GlobalMapWidth + WrappedX;

	// Return the number at this position
	if (GlobalGridNumbers.IsValidIndex(GlobalIdx))
	{
		return GlobalGridNumbers[GlobalIdx];
	}
    
	return 0;
}

// ========================================
// SCARY NUMBER DROP SYSTEM
// ========================================

/**
 * Gets a 3x3 group of indices around a center point.
 * Used for visual grouping or special effects.
 */
TArray<int32> ATerminalActor::Get3x3Group(int32 CenterIndex) const
{
	TArray<int32> Group;
	
	// Convert center index to 2D coordinates
	int32 CenterX = CenterIndex % GlobalMapWidth;
	int32 CenterY = CenterIndex / GlobalMapWidth;

	// Gather 3x3 grid around center
	for (int32 y = CenterY - 1; y <= CenterY + 1; ++y)
	{
		for (int32 x = CenterX - 1; x <= CenterX + 1; ++x)
		{
			// Only add if within bounds
			if (x >= 0 && x < GlobalMapWidth && y >= 0 && y < GlobalMapHeight)
			{
				Group.Add(y * GlobalMapWidth + x);
			}
		}
	}
	
	return Group;
}

/**
 * Handles a player "dropping" a snake of numbers onto a progress bar.
 * 
 * Process:
 * 1. Calculate value of each number in the snake
 * 2. Apply 4x multiplier for scary (red) numbers
 * 3. Consume scary numbers (turn them back to normal)
 * 4. Replace eaten numbers with fresh random numbers
 * 5. Apply total value to the selected progress bar
 */
void ATerminalActor::HandleScaryDrop(const TArray<int32>& TileIndices, int32 BarIndex)
{
	float TotalValueFromSnake = 0.0f;

	// Process each tile in the snake
	for (int32 ScreenIdx : TileIndices)
	{
		// ========================================
		// Step 1: Convert Screen Index to Global Index
		// ========================================
		int32 GlobalIdx = GetGlobalIndexFromScreenIndex(ScreenIdx);
        
		if (GlobalGridNumbers.IsValidIndex(GlobalIdx))
		{
			// ========================================
			// Step 2: Get Number Value (1-9)
			// ========================================
			int32 NumberValue = GlobalGridNumbers[GlobalIdx];
            
			// ========================================
			// Step 3: Calculate Base Progress
			// ========================================
			// Higher numbers = more progress
			// Example: a '9' gives 0.045, a '1' gives 0.005
			float ProgressContribution = (float)NumberValue * 0.005f;

			// ========================================
			// Step 4: Apply Scary Multiplier
			// ========================================
			if (ScaryActive.IsValidIndex(GlobalIdx) && ScaryActive[GlobalIdx])
			{
				ProgressContribution *= 4.0f; // 4x bonus for red numbers!
				ScaryActive[GlobalIdx] = false; // "Consume" the scary state
			}

			// ========================================
			// Step 5: Replace with New Number
			// ========================================
			// Refresh the tile so player can't eat the same number twice
			GlobalGridNumbers[GlobalIdx] = FMath::RandRange(1, 9);
            
			// Add to total value
			TotalValueFromSnake += ProgressContribution;
		}
	}

	// ========================================
	// Step 6: Apply Total Value to Bar
	// ========================================
	PendingChunkValue = TotalValueFromSnake;
	ApplChunkToBar(BarIndex);

	// ========================================
	// Step 7: Refresh UI
	// ========================================
	// Force the UI to update to show new numbers
	OnGridScrolled();
}

/**
 * Checks if a specific bar has reached 100%.
 */
bool ATerminalActor::IsBarFull(int32 BarIndex) const
{
	if (ProgressBars.IsValidIndex(BarIndex))
	{
		return ProgressBars[BarIndex] >= 1.0f;
	}
	return false;
}