#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerminalActor.generated.h"

/**
 * Terminal Actor - Core gameplay system for the file refinement minigame.
 * 
 * This actor represents a retro CRT terminal that the player interacts with to refine files.
 * It manages:
 * - An infinite wrapping grid of numbers (1-9) that the player navigates
 * - Four progress bars that must be filled to complete a file
 * - "Scary" red numbers scattered across the grid that provide bonuses
 * - A proximity sensor system that detects nearby scary numbers
 * - Day/file completion tracking
 * 
 * Gameplay Flow:
 * 1. Player starts a day (multiple files to complete)
 * 2. Player creates "snakes" of numbers on the grid
 * 3. Snakes are dropped into progress bars to fill them
 * 4. When all 4 bars are full, one file is complete
 * 5. Process repeats until all files for the day are done
 */
UCLASS()
class PROJECT_REFINEMENT_API ATerminalActor : public AActor
{
	GENERATED_BODY()

public:
	ATerminalActor();

	virtual void BeginPlay() override;
	
	/**
	 * Called when all four progress bars reach 100%.
	 * Virtual so it can be overridden in Blueprint for custom behavior.
	 */
	virtual void OnAllBarsFull();

	/**
	 * Exits terminal mode and returns camera control to the player character.
	 * Called when player presses the exit button or interaction key while seated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ExitTerminalMode();

	// ========================================
	// Player Interaction Events
	// ========================================
	
	/**
	 * Blueprint event called when the player sits down at the terminal.
	 * Use this to show UI, play sounds, start animations, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Interaction")
	void OnPlayerInteract();

	/**
	 * Blueprint event called when the player stands up from the terminal.
	 * Use this to hide UI, stop sounds, reset state, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Interaction")
	void OnPlayerExit();

	// ========================================
	// Components
	// ========================================
	
	/** The CRT monitor mesh component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* CRTMonitor;

	/**
	 * Camera positioned at the terminal screen.
	 * Player's view switches to this when interacting with the terminal.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal UI")
	class UCameraComponent* TerminalCamera;

	// ========================================
	// Grid System (Local/Screen)
	// ========================================
	
	/** Current visible grid numbers (10x10 window into the global map) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<int32> GridNumbers;

	/** Indices of prime numbers in the current visible grid (2, 3, 5, 7) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<int32> PrimeIndices;

	/** Which prime numbers are currently highlighted (for visual feedback) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<bool> HighlightedPrimes;

	/** Width of the visible grid window (default: 10) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GridWidth = 10;

	/** Height of the visible grid window (default: 10) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GridHeight = 10;

	/** Legacy scroll offset (kept for compatibility, use ScrollX/ScrollY instead) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ScrollOffsetX = 0;

	/** Legacy scroll offset (kept for compatibility, use ScrollX/ScrollY instead) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ScrollOffsetY = 0;

	// ========================================
	// Grid Functions
	// ========================================
	
	/**
	 * Generates the global infinite grid with random numbers.
	 * Also spawns "scary" numbers in a sector pattern (one per 50x50 block).
	 */
	UFUNCTION(BlueprintCallable)
	void GenerateGrid();

	/**
	 * Checks if a number is prime (2, 3, 5, or 7).
	 * Used for special highlighting/mechanics.
	 */
	UFUNCTION(BlueprintCallable)
	bool IsPrime(int32 Number) const;

	/**
	 * Randomly selects and activates one non-scary prime number as scary.
	 * Used to increase difficulty/tension over time.
	 */
	UFUNCTION(BlueprintCallable)
	void HighlightRandomPrime();

	/**
	 * Checks if a screen-space tile index is currently scary (red).
	 * 
	 * @param ScreenIndex - Index in the visible 10x10 grid (0-99)
	 * @return true if that tile is a scary number
	 */
	UFUNCTION(BlueprintPure)
	bool IsIndexScary(int32 ScreenIndex) const;

	/**
	 * Gets a 3x3 group of indices around a center point.
	 * Used for group operations or visual effects.
	 * 
	 * @param CenterIndex - The center tile index in global coordinates
	 * @return Array of up to 9 indices in the group
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal|Grid")
	TArray<int32> Get3x3Group(int32 CenterIndex) const;

	/**
	 * Gets the number at a specific grid coordinate.
	 * Handles wrapping for the infinite grid.
	 * 
	 * @param GridX - X coordinate (can be any value, wraps automatically)
	 * @param GridY - Y coordinate (can be any value, wraps automatically)
	 * @return The number (1-9) at that position
	 */
	UFUNCTION(BlueprintPure)
	int32 GetGridNumber(int32 GridX, int32 GridY) const;

	/**
	 * Blueprint event called whenever the grid scrolls.
	 * Use this to update UI, refresh displayed numbers, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal|Events")
	void OnGridScrolled();

	// ========================================
	// Progress Bar System
	// ========================================
	
	/** Current fill level of the four progress bars (0.0 to 1.0 each) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> ProgressBars;

	/**
	 * Resets all progress bars to 0% and broadcasts the update to UI.
	 * Called when starting a new file refinement.
	 */
	UFUNCTION(BlueprintCallable)
	void ResetProgressBars();

	/**
	 * Checks if a specific progress bar is available to receive input.
	 * A bar is unavailable if it's on cooldown OR already full.
	 * 
	 * @param BarIndex - Which bar to check (0-3)
	 * @return true if the bar can accept more progress
	 */
	UFUNCTION(BlueprintPure, Category = "Cooldown")
	bool IsBarAvailable(int32 BarIndex) const;

	/**
	 * Checks if a specific bar has reached 100%.
	 * 
	 * @param BarIndex - Which bar to check (0-3)
	 * @return true if the bar is completely full
	 */
	UFUNCTION(BlueprintPure, Category = "Progress")
	bool IsBarFull(int32 BarIndex) const;

	/**
	 * Returns the master progress (average of all four bars).
	 * When this reaches 1.0, the current file is complete.
	 * 
	 * @return Progress from 0.0 to 1.0
	 */
	UFUNCTION(BlueprintPure, Category = "Refinement")
	float GetMasterProgress() const;

	/**
	 * Blueprint event called when any progress bar value changes.
	 * Use this to update progress bar visuals.
	 * 
	 * @param BarIndex - Which bar changed (0-3)
	 * @param NewValue - New fill amount (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnProgressUpdated(int32 BarIndex, float NewValue);

	// ========================================
	// Chunk System (Snake Drops)
	// ========================================
	
	/**
	 * The value waiting to be assigned to a progress bar.
	 * Set by HandleScaryDrop, consumed by ApplChunkToBar.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Progress")
	float PendingChunkValue = 0.f;

	/**
	 * Checks if there's a chunk waiting to be placed.
	 * 
	 * @return true if PendingChunkValue > 0
	 */
	UFUNCTION(BlueprintPure, Category = "Progress")
	bool HasPendingChunk() const { return PendingChunkValue > 0.f; }

	/**
	 * Applies the pending chunk value to a specific progress bar.
	 * Clamps the bar to 1.0 max, checks for file completion.
	 * 
	 * @param BarIndex - Which bar to add progress to (0-3)
	 */
	UFUNCTION(BlueprintCallable, Category = "Progress")
	void ApplChunkToBar(int32 BarIndex);

	/**
	 * Handles a player "dropping" a snake of numbers onto a progress bar.
	 * Calculates total value based on number values and scary multipliers.
	 * 
	 * @param TileIndices - Screen-space indices of tiles in the snake
	 * @param BarIndex - Which progress bar to apply the value to
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void HandleScaryDrop(const TArray<int32>& TileIndices, int32 BarIndex);

	/**
	 * Blueprint event called when a chunk is calculated and ready.
	 * 
	 * @param ChunkValue - The calculated progress value
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Progress")
	void OnChunkReady(float ChunkValue);

	/**
	 * Blueprint event called after a chunk is applied to a bar.
	 * Use this for sound effects, particles, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Progress")
	void OnChunkConsumed();

	// ========================================
	// Cooldown System
	// ========================================
	
	/** Tracks which bars are currently in cooldown (can't accept input) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooldown")
	TArray<bool> bBarCoolingDown;

	/** Time remaining for each bar's cooldown */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooldown")
	TArray<float> BarCooldownRemaining;

	/** How long bars stay on cooldown after being filled (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooldown")
	float BarCooldownSeconds = 2.5f;

	/**
	 * Gets the cooldown progress for a bar (0.0 = done, 1.0 = just started).
	 * Use this to display cooldown timer UI.
	 * 
	 * @param BarIndex - Which bar to check (0-3)
	 * @return Cooldown ratio from 0.0 to 1.0
	 */
	UFUNCTION(BlueprintPure, Category = "Cooldown")
	float GetBarCooldownRatio(int32 BarIndex) const;

	/**
	 * Blueprint event called when a bar enters cooldown.
	 * 
	 * @param BarIndex - Which bar (0-3)
	 * @param Duration - How long the cooldown will last
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cooldown")
	void OnBarCooldownStarted(int32 BarIndex, float Duration);

	/**
	 * Blueprint event called when a bar's cooldown finishes.
	 * 
	 * @param BarIndex - Which bar is now available again
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cooldown")
	void OnBarCooldownEnded(int32 BarIndex);

	// ========================================
	// Day/File Management
	// ========================================
	
	/** How many files must be refined to complete a day */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day")
	int32 FilesPerDay = 2;

	/** How many files have been completed in the current day */
	UPROPERTY(BlueprintReadWrite, Category = "Day")
	int32 FilesRefinedCount = 0;

	/** Legacy files completed counter (kept for compatibility) */
	UPROPERTY(BlueprintReadOnly, Category = "Day")
	int32 FilesCompleted = 0;

	/** Whether a workday is currently active */
	UPROPERTY(BlueprintReadOnly, Category = "Day")
	bool bDayActive = false;

	/**
	 * Starts a new workday.
	 * Resets counters, generates new grid, broadcasts event.
	 */
	UFUNCTION(BlueprintCallable, Category = "Day")
	void StartDay();

	/**
	 * Ends the current workday.
	 * Sets bDayActive to false, broadcasts completion event.
	 */
	UFUNCTION(BlueprintCallable, Category = "Day")
	void EndDay();

	/**
	 * Called when a single file reaches 100% completion.
	 * Updates counter, checks if day is complete.
	 */
	UFUNCTION(BlueprintCallable, Category = "Refinement")
	void OnFileWorkComplete();

	/**
	 * Blueprint event called when the day begins.
	 * Use this to show day start UI, play music, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Day")
	void OnDayStarted();

	/**
	 * Blueprint event for file selection screen.
	 * Called between files to let player choose next file type.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Refinement")
	void BP_OnShowFileSelection();

	/**
	 * Blueprint event called when a file is completed.
	 * 
	 * @param FilesDone - Number of files completed so far
	 * @param FilesTarget - Total files needed for the day
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Day")
	void OnFileCompleted(int32 FilesDone, int32 FilesTarget);

	/**
	 * Blueprint event called when the entire day is finished.
	 * 
	 * @param DayDurationSeconds - How long the day took in real time
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Refinement")
	void BP_OnDayComplete(float DayDurationSeconds);

	/**
	 * Blueprint event called when the day ends.
	 * Use this for end-of-day UI, stats screen, etc.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Day")
	void OnDayCompleted();

	// ========================================
	// Scary Number System
	// ========================================
	
	/**
	 * Tracks which tiles in the global grid are currently "scary" (red).
	 * Scary numbers provide 4x progress bonus when consumed.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scary")
	TArray<bool> ScaryActive;

	/**
	 * Gets the proximity sensor value (0.0 = far, 1.0 = very close).
	 * Indicates how close the nearest scary number is to the player's view.
	 * Use this to drive anxiety/stress UI, sound effects, screen shake, etc.
	 * 
	 * @return Distance ratio from 0.0 (far/none) to 1.0 (very close)
	 */
	UFUNCTION(BlueprintPure, Category = "Scary|Sensor")
	float GetSensorProximityValue() const;

	/** Maximum distance (in tiles) for scary number detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scary|Sensor")
	float MaxSensorDistance = 20.0f;

	/**
	 * Blueprint event called when a group of tiles is cleared.
	 * 
	 * @param GroupIndices - The indices of tiles that were cleared
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnGroupCleared(const TArray<int32>& GroupIndices);

	// ========================================
	// Infinite Grid / Scrolling System
	// ========================================
	
	/** The complete infinite grid of numbers (1000x1000 by default) */
	UPROPERTY()
	TArray<int32> GlobalGridNumbers;

	/** Width of the global grid (creates wrapping/pac-man effect) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Infinite")
	int32 GlobalMapWidth = 1000;

	/** Height of the global grid (creates wrapping/pac-man effect) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Infinite")
	int32 GlobalMapHeight = 1000;

	/** Current horizontal scroll position in the global grid */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terminal|Infinite")
	int32 ScrollX = 0;

	/** Current vertical scroll position in the global grid */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terminal|Infinite")
	int32 ScrollY = 0;

	/** Mouse/trackball sensitivity multiplier for scrolling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Infinite")
	float ScrollSensitivity = 0.5f;

	/** Accumulates sub-pixel X movement for smooth scrolling */
	UPROPERTY(BlueprintReadOnly, Category = "Terminal|Movement")
	float AccumulatorX;

	/** Accumulates sub-pixel Y movement for smooth scrolling */
	UPROPERTY(BlueprintReadOnly, Category = "Terminal|Movement")
	float AccumulatorY;

	/**
	 * Applies mouse/trackball movement to scroll the grid.
	 * Handles sub-pixel accumulation and wrapping.
	 * 
	 * @param AxisX - Horizontal input (-1.0 to 1.0)
	 * @param AxisY - Vertical input (-1.0 to 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "Terminal|Input")
	void ApplyTrackballInput(float AxisX, float AxisY);

	/**
	 * Converts a screen-space index (0-99) to a global grid index.
	 * Handles wrapping for the infinite grid.
	 * 
	 * @param ScreenIndex - Index in the visible 10x10 window
	 * @return Global index in the 1000x1000 grid
	 */
	UFUNCTION(BlueprintPure, Category = "Terminal|Infinite")
	int32 GetGlobalIndexFromScreenIndex(int32 ScreenIndex) const;

	// ========================================
	// Debug / Legacy
	// ========================================
	
	/** Legacy grid data array (kept for compatibility) */
	UPROPERTY(BlueprintReadOnly)
	TArray<int32> GridData;

protected:
	/** Timestamp when the current day started (for duration calculation) */
	float DayStartTime;

private:
	/** Timer for periodic prime highlighting */
	FTimerHandle HighlightTimerHandle;
	
	/** Timer for stress/difficulty increases */
	FTimerHandle StressTimerHandle;

	/**
	 * Helper function to find distance to the nearest scary number.
	 * Used by GetSensorProximityValue.
	 * 
	 * @return Distance in tiles to nearest scary number
	 */
	float GetDistanceToNearestScary() const;

	/**
	 * Checks if all four progress bars have reached 100%.
	 * 
	 * @return true if all bars are full
	 */
	bool AllBarsFull() const;
};