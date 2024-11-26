#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define PLAYER_WIDTH 25
#define PLAYER_HEIGHT 25
#define GRAVITY 0.8
#define JUMP_STRENGTH -10
#define MAX_VELOCITY 10

Mix_Music *backgroundMusic = NULL;

// Platform structure
typedef struct {
    float x, y, width, height;
    float velX;  // Horizontal velocity for moving platforms
} Platform;

// Player structure
typedef struct {
    float x, y;  // Position
    float velX, velY;  // Velocity
    int onGround;  // Is the player on a platform?
} Player;

// Trap structure
typedef struct {
    float x, y;  // Position
    float velX, velY;  // Velocity
    int size;  // Size of the triangle (base and height)
} Trap;

// Surprise Trap structure
typedef struct {
    float x, y;    // Position
    int size;      // Size of the triangle (base and height)
    int visible;   // Is the trap visible?
    int platformIndex;  // Platform index where it spawns
} SurpriseTrap;

typedef struct {
    float x, y;    // Position
    float radius;  // Radius of the circle
    int active;    // Is the buff active?
    int collected; // Has the buff been collected?
    int used;      // Has the buff effect been used?
    int type;      // 0 = Score, 1 = Defense, 2 = Shooter
} Buff;

typedef struct {
    float x, y, velX;
    int active;
} Bullet;

Bullet bullets[10];  // Array of bullets

Buff buffs[3];  // Array to hold three buffs
int score = 0;  // Score counter
int defenseBuff = 0;  // Stores defense status
int shooterBuff = 0;  // Stores shooter ability status


// Exit door structure
typedef struct {
    float x, y;     // Position
    float width, height;  // Size of the door
} ExitDoor;


SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

// Static platforms + Moving platforms (9 to 14)
Platform platforms[] = {
    //static platform
    {0, 160, 100, 10, 0}, {150, 160, 50, 10, 0}, {190, 0, 10, 160, 0},
    {190, 440, 210, 10, 0}, {400, 440, 10, 590, 0}, {410, 590, 120, 10, 0},
    {530, 550, 10, 50, 0}, {530, 540, 230, 10, 0}, {640, 0, 10, 170, 0}, {640, 170, 120, 10, 0},  
    //this part is moving platform
    {740, 450, 40, 10, 2.0},
    {620, 390, 40, 10, 2.0}, {750, 360, 40, 10, 2.0}, {640, 310, 40, 10, 2.0},
    {670, 240, 40, 10, 2.0}, {600, 500, 40, 10, 2.0}
};
const int NUM_PLATFORMS = sizeof(platforms) / sizeof(platforms[0]);

Player player = {50, 100, 0, 0, 0};  // Player starts at (50, 100)

// Initialize the exit door on platform at {640, 170, 120, 10, 0}
ExitDoor exitDoor = {640 + 40, 170 - 30, 40, 30};  // Centered on the platform


// Initialize traps
Trap traps[5] = {
    {100, 300, 3, 2, 30},
    {400, 200, -2, 3, 30},
    {600, 400, -3, -2, 30},
    {200, 100, 2, -3, 30},
    {500, 300, -3, 2, 30}
};
const int NUM_TRAPS = sizeof(traps) / sizeof(traps[0]);

// Surprise traps
SurpriseTrap surpriseTraps[3];  // Three surprise traps
const int NUM_SURPRISE_TRAPS = sizeof(surpriseTraps) / sizeof(surpriseTraps[0]);
int surpriseTimer = 0;  // Timer for activating surprise traps

// Initialize SDL and create window/renderer
int initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { // Add SDL_INIT_AUDIO
        printf("SDL Initialization Error: %s\n", SDL_GetError());
        return 0;
    }
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }
    if (Mix_Init(MIX_INIT_MP3) == 0) {
        printf("SDL_mixer could not initialize! Mix_Error: %s\n", Mix_GetError());
        return -1;
    }
    if (TTF_Init() != 0) {
        printf("TTF Initialization Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 0;
    }
    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        printf("SDL_mixer could not open audio! Mix_Error: %s\n", Mix_GetError());
        return -1;
    }
    Mix_Music *bgMusic = Mix_LoadMUS("media/musicAudio.mp3");
    if (bgMusic == NULL) {
        printf("Failed to load music! Mix_Error: %s\n", Mix_GetError());
        return -1;
    }
    Mix_PlayMusic(bgMusic, -1);

    window = SDL_CreateWindow("SDL2 Platformer",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        printf("Window Creation Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer Creation Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    for (int i = 0; i < NUM_SURPRISE_TRAPS; ++i) {
        surpriseTraps[i].size = 30;  // Default size for surprise traps
        surpriseTraps[i].visible = 0;  // Start as invisible
    }

    srand(time(NULL));  // Seed random number generator
    return 1;
}


void initializeBuffs() {
    for (int i = 0; i < 3; ++i) {
        buffs[i].x = rand() % (WINDOW_WIDTH - 50) + 25;  // Random X within window
        buffs[i].y = rand() % (WINDOW_HEIGHT - 50) + 25; // Random Y within window
        buffs[i].radius = 15;
        buffs[i].active = 1; // Buff is active
        buffs[i].collected = 0; // Buff has not been collected
        buffs[i].used = 0; // Buff effect has not been used
        buffs[i].type = i;  // Each buff has a unique type (0 = Score, 1 = Defense, 2 = Shooter)
    }
}

// Clean up SDL resources
void cleanupSDL() {
    if (backgroundMusic) {
        Mix_HaltMusic();
        Mix_FreeMusic(backgroundMusic);
        backgroundMusic = NULL;
    }
    Mix_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

// Function to render text
void renderText(const char *text, int x, int y) {
    TTF_Font *font = TTF_OpenFont("Arial.ttf", 24); // Specify a path to your font file
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }

    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, white);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect destRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &destRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);
}

// Function to show the main menu
void mainMenu() {
    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;

                // Check if "Start Game" is clicked
                if (x >= 300 && x <= 500 && y >= 200 && y <= 250) {
                    printf("Starting Game...\n");
                    running = 0; // Exit menu loop to start the game
                }
                // Check if "Instructions" is clicked
                else if (x >= 300 && x <= 500 && y >= 300 && y <= 350) {
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Instructions", "Use A to move left, D to move right, and SPACE to jump.", window);
                }
                // Check if "Exit" is clicked
                else if (x >= 300 && x <= 500 && y >= 400 && y <= 450) {
                    running = 0; // Exit the application
                }
            }
        }

        // Render the menu
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);
        renderText("[_Main Menu_]", 350, 100);
        renderText("Start Game", 350, 200);
        renderText("Instructions", 350, 300);
        renderText("Exit", 350, 400);
        SDL_RenderPresent(renderer);
    }
}

void renderScore(int score) {
    TTF_Font *font = TTF_OpenFont("Arial.ttf", 24); // Specify a path to your font file
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }

    SDL_Color white = {255, 255, 255, 255};
    char scoreText[50];
    snprintf(scoreText, sizeof(scoreText), "Score: %d", score);

    SDL_Surface *surface = TTF_RenderText_Solid(font, scoreText, white);
    if (!surface) {
        printf("Failed to create text surface: %s\n", TTF_GetError());
        TTF_CloseFont(font);
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        printf("Failed to create text texture: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        TTF_CloseFont(font);
        return;
    }

    SDL_Rect destRect = {10, WINDOW_HEIGHT - 40, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &destRect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    TTF_CloseFont(font);
}

void showScore(int finalScore) {
    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            if (event.type == SDL_KEYDOWN) {
                running = 0; // Exit the score display on key press
            }
        }

        // Render the score screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);
        renderText("Game Over! Your Score:", 250, 200);
        
        char scoreText[50];
        snprintf(scoreText, sizeof(scoreText), "Score: %d", finalScore);
        renderText(scoreText, 350, 300);
        
        renderText("Press any key to return to the main menu.", 200, 400);
        SDL_RenderPresent(renderer);
        
        SDL_Delay(16); // Delay for frame rate control
    }
}


// Check if the player is colliding with a platform
int checkCollision(Player *p, Platform *plat) {
    return (p->x < plat->x + plat->width &&
            p->x + PLAYER_WIDTH > plat->x &&
            p->y < plat->y + plat->height &&
            p->y + PLAYER_HEIGHT > plat->y);
}

int checkBuffCollision(Player *p, Buff *b) {
    float dx = (p->x + PLAYER_WIDTH / 2) - b->x;
    float dy = (p->y + PLAYER_HEIGHT / 2) - b->y;
    float distance = sqrt(dx * dx + dy * dy);

    return distance < b->radius + PLAYER_WIDTH / 2;
}


// Reset the player's position
void resetPlayer() {
    player.x = 50;
    player.y = 100;
    player.velX = 0;
    player.velY = 0;
}

// Check if the player collides with a trap
int checkTrapCollision(Player *p, Trap *t) {
    return (p->x < t->x + t->size &&
            p->x + PLAYER_WIDTH > t->x &&
            p->y < t->y + t->size &&
            p->y + PLAYER_HEIGHT > t->y);
}

// Check if the player collides with a surprise trap
int checkSurpriseTrapCollision(Player *p, SurpriseTrap *t) {
    if (!t->visible) return 0;
    return (p->x < t->x + t->size &&
            p->x + PLAYER_WIDTH > t->x &&
            p->y < t->y + t->size &&
            p->y + PLAYER_HEIGHT > t->y);
}

// Move the traps and make them bounce within the window
void moveTraps() {
    for (int i = 0; i < NUM_TRAPS; ++i) {
        traps[i].x += traps[i].velX;
        traps[i].y += traps[i].velY;

        if (traps[i].x < 0 || traps[i].x + traps[i].size > WINDOW_WIDTH) {
            traps[i].velX *= -1;
        }
        if (traps[i].y < 0 || traps[i].y + traps[i].size > WINDOW_HEIGHT) {
            traps[i].velY *= -1;
        }
    }
}

// Activate surprise traps randomly on platforms
void activateSurpriseTraps() {
    surpriseTimer++;
    if (surpriseTimer >= 300) {  // Activate every 5 seconds (assuming ~60 FPS)
        surpriseTimer = 0;  // Reset timer

        for (int i = 0; i < NUM_SURPRISE_TRAPS; ++i) {
            surpriseTraps[i].platformIndex = rand() % NUM_PLATFORMS;
            Platform *p = &platforms[surpriseTraps[i].platformIndex];

            // Ensure the trap size is initialized (e.g., 30 pixels)
            surpriseTraps[i].size = 30;

            // Position the trap on top of the selected platform
            surpriseTraps[i].x = p->x + p->width / 2 - surpriseTraps[i].size / 2;
            surpriseTraps[i].y = p->y - surpriseTraps[i].size;

            // Make the trap visible
            surpriseTraps[i].visible = 1;

            // Debugging output to ensure correct values
            printf("SurpriseTrap %d: x = %.2f, y = %.2f, visible = %d\n",
                   i, surpriseTraps[i].x, surpriseTraps[i].y, surpriseTraps[i].visible);
        }
    }
}


// Move platforms 9 to 14 left and right between 600 and 800
void moveMovingPlatforms() {
    for (int i = 9; i <= 14; ++i) {
        platforms[i].x += platforms[i].velX;

        // Reverse direction if the platform hits the boundaries
        if (platforms[i].x <= 600 || platforms[i].x + platforms[i].width >= 800) {
            platforms[i].velX *= -1;
        }
    }
}

void initializeBullets() {
    for (int i = 0; i < 10; i++) bullets[i].active = 0;
}

void shootBullet() {
    int bulletShot = 0;
    for (int i = 0; i < 10; i++) {
        if (!bullets[i].active) {
            bullets[i].x = player.x + PLAYER_WIDTH / 2;
            bullets[i].y = player.y;
            bullets[i].velX = 8;  // Speed of bullet
            bullets[i].active = 1;
            bulletShot = 1;
            break;
        }
    }
    if (!bulletShot) {
        printf("No bullets available!\n");
    }
}


void moveBullets() {
    for (int i = 0; i < 10; i++) {
        if (bullets[i].active) {
            bullets[i].x += bullets[i].velX;
            if (bullets[i].x > WINDOW_WIDTH) bullets[i].active = 0;  // Deactivate bullet
        }
    }
}

void checkBulletCollisions() {
    for (int i = 0; i < 10; i++) {
        if (bullets[i].active) {
            for (int j = 0; j < NUM_TRAPS; j++) {
                if (checkTrapCollision(&player, &traps[j])) {
                    traps[j].x = -100; // Remove trap from screen
                    bullets[i].active = 0;
                    printf("Trap destroyed by bullet!\n");
                    break;
                }
            }
        }
    }
}


void renderBullets() {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red bullets
    for (int i = 0; i < 10; i++) {
        if (bullets[i].active) {
            SDL_Rect bulletRect = {bullets[i].x, bullets[i].y, 5, 5};
            SDL_RenderFillRect(renderer, &bulletRect);
        }
    }
}



// Apply gravity and handle collisions with platforms
void applyPhysics() {
    player.velY += GRAVITY;

    if (player.velY > MAX_VELOCITY) player.velY = MAX_VELOCITY;

    player.x += player.velX;
    player.y += player.velY;

    player.onGround = 0;

    for (int i = 0; i < NUM_PLATFORMS; ++i) {
        if (checkCollision(&player, &platforms[i])) {
            player.y = platforms[i].y - PLAYER_HEIGHT;
            player.velY = 0;
            player.onGround = 1;

            // Adjust the player's position if on a moving platform
            if (i >= 9 && i <= 14) {
                player.x += platforms[i].velX;
            }
            break;
        }
    }

    if (player.x < 0) player.x = 0;
    if (player.x + PLAYER_WIDTH > WINDOW_WIDTH) player.x = WINDOW_WIDTH - PLAYER_WIDTH;

    if (player.y > WINDOW_HEIGHT) resetPlayer();
}

int checkExitCollision(Player *p, ExitDoor *door) {
    return (p->x < door->x + door->width &&
            p->x + PLAYER_WIDTH > door->x &&
            p->y < door->y + door->height &&
            p->y + PLAYER_HEIGHT > door->y);
         
}

////////////////////////////
void renderCircle(SDL_Renderer *renderer, int x, int y, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; // Horizontal distance from center
            int dy = radius - h; // Vertical distance from center
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
            }
        }
    }
}


///////////////////////////////////
// Render the game objects
void renderGame() {
    SDL_SetRenderDrawColor(renderer, 64, 64, 64, 64);  // Grey background
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Green for the exit door
    SDL_Rect exitRect = {exitDoor.x, exitDoor.y, exitDoor.width, exitDoor.height};
    SDL_RenderFillRect(renderer, &exitRect);


    SDL_SetRenderDrawColor(renderer, 178, 34, 34, 0);  // Firebrick color platforms
    for (int i = 0; i < NUM_PLATFORMS; ++i) {
        SDL_Rect rect = {platforms[i].x, platforms[i].y, platforms[i].width, platforms[i].height};
        SDL_RenderFillRect(renderer, &rect);
    }

    SDL_SetRenderDrawColor(renderer, 147, 112, 219, 0);  // medium purple player
    SDL_Rect playerRect = {player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_RenderFillRect(renderer, &playerRect);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Black traps
    for (int i = 0; i < NUM_TRAPS; ++i) {
        SDL_RenderDrawLine(renderer, traps[i].x, traps[i].y,
                           traps[i].x + traps[i].size / 2, traps[i].y + traps[i].size);
        SDL_RenderDrawLine(renderer, traps[i].x + traps[i].size / 2, traps[i].y + traps[i].size,
                           traps[i].x + traps[i].size, traps[i].y);
        SDL_RenderDrawLine(renderer, traps[i].x + traps[i].size, traps[i].y, traps[i].x, traps[i].y);
    }

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red surprise traps
    for (int i = 0; i < NUM_SURPRISE_TRAPS; ++i) {
        if (surpriseTraps[i].visible) {
            SDL_RenderDrawLine(renderer, surpriseTraps[i].x, surpriseTraps[i].y,
                           surpriseTraps[i].x + surpriseTraps[i].size / 2, surpriseTraps[i].y + surpriseTraps[i].size);
            SDL_RenderDrawLine(renderer, surpriseTraps[i].x + surpriseTraps[i].size / 2, surpriseTraps[i].y + surpriseTraps[i].size,
                           surpriseTraps[i].x + surpriseTraps[i].size, surpriseTraps[i].y);
            SDL_RenderDrawLine(renderer, surpriseTraps[i].x + surpriseTraps[i].size, surpriseTraps[i].y,
                           surpriseTraps[i].x, surpriseTraps[i].y);
        }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // White color
    for (int i = 0; i < 3; ++i) {
        if (buffs[i].active && !buffs[i].collected) {
            switch (buffs[i].type) {
                case 0: SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255); break;  // Gold for Score
                case 1: SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); break;  // Cyan for Defense
                case 2: SDL_SetRenderDrawColor(renderer, 255, 69, 0, 255); break;   // Orange-Red for Shooter
            }
            renderCircle(renderer, (int)buffs[i].x, (int)buffs[i].y, (int)buffs[i].radius);
        }
    }

    renderScore(score);
    SDL_RenderPresent(renderer);
}

///////////////////////////////////
// Handle player input
void handleInput(SDL_Event *event) {
    if (event->type == SDL_KEYDOWN) {
        switch (event->key.keysym.sym) {
            case SDLK_a:
                player.velX = -5;
                break;
            case SDLK_d:
                player.velX = 5;
                break;
            case SDLK_SPACE:
                if (player.onGround) player.velY = JUMP_STRENGTH;
                break;
        }
    } else if (event->type == SDL_KEYUP) {
        if (event->key.keysym.sym == SDLK_a || event->key.keysym.sym == SDLK_d) {
            player.velX = 0;
        }
    }
}

///////////////////////////////
// Main game loop
void gameLoop() {
    SDL_Event event;
    int running = 1;

    // Initialize buffs
    int defenseBuffActive = 0;
    int shooterBuffActive = 0;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            handleInput(&event);  // Process user input
        }

        // Update game logic
        applyPhysics();
        moveTraps();
        moveMovingPlatforms();
        activateSurpriseTraps();
        moveBullets();
        checkBulletCollisions();

        // Check for collisions with traps
        for (int i = 0; i < NUM_TRAPS; ++i) {
            if (checkTrapCollision(&player, &traps[i])) {
                if (shooterBuffActive) {
                    // Shooter buff is active, remove the trap
                    traps[i].x = -100; // Move trap off-screen
                    printf("Trap destroyed by shooter buff!\n");
                    shooterBuffActive = 0; // Mark shooter buff as inactive
                } else if (defenseBuffActive) {
                    // Defense buff is active, player is protected
                    printf("Collision with trap! Defense buff active, player is safe.\n");
                    defenseBuffActive = 0; // Mark defense buff as inactive
                } else {
                    // No buffs active, player resets
                    printf("Collision with trap! Resetting player.\n");
                    resetPlayer();
                    score -= 1; // Reduce score by 1
                }
                break; // Exit the loop after handling the first collision
            }
        }

        // Check for collisions with surprise traps
        for (int i = 0; i < NUM_SURPRISE_TRAPS; ++i) {
            if (checkSurpriseTrapCollision(&player, &surpriseTraps[i])) {
                if (shooterBuffActive) {
                    // Shooter buff is active, remove the surprise trap
                    surpriseTraps[i].visible = 0; // Hide the surprise trap
                    printf("Surprise trap destroyed by shooter buff!\n");
                    shooterBuffActive = 0; // Mark shooter buff as inactive
                } else if (defenseBuffActive) {
                    // Defense buff is active, player is protected
                    printf("Collision with surprise trap! Defense buff active, player is safe.\n");
                    defenseBuffActive = 0; // Mark defense buff as inactive
                } else {
                    // No buffs active, player resets
                    printf("Collision with surprise trap! Resetting player.\n");
                    resetPlayer();
                    score -= 1; // Reduce score by 1
                }
                break; // Exit the loop after handling the first collision
            }
        }

        // Check if the player falls from the platform
        if (player.y > WINDOW_HEIGHT) {
            resetPlayer();
            score -= 1; // Reduce score by 1
        }

        // Check for collisions with buffs
        for (int i = 0; i < 3; ++i) {
            if (buffs[i].active && !buffs[i].collected && checkBuffCollision(&player, &buffs[i])) {
                buffs[i].collected = 1; // Mark the buff as collected
                buffs[i].active = 0; // Deactivate the buff

                // Apply buff effect only if it hasn't been used
                if (!buffs[i].used) {
                    buffs[i].used = 1; // Mark the buff effect as used

                    switch (buffs[i].type) {
                        case 0:
                            score += 10; // Score buff
                            printf("Luck with you. Got extra 10 score. You collected a score buff! Score: %d\n", score);
                            break;
                        case 1:
                            defenseBuffActive = 1; // Defense buff
                            printf("Defense Buff collected! Defense activated. At least you didn't see starting point once\n");
                            break;
                        case 2:
                            shooterBuffActive = 1; // Shooter buff
                            printf("Shooter Buff collected! Bullets activated.Pity you only one bullet\n");
                            break;
                    }
                }
            }
        }

        // Check if player reaches the exit
        if (checkExitCollision(&player, &exitDoor)) {
            if (score < 70) {
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Game Over", "Your score below 70! Better Luck next time", window);
                printf("Game Over! Returning to main menu.\n");
                mainMenu(); // Return to the main menu
                running = 0; // Exit the game loop
            } else {
                printf("Player reached the exit! You win the game! Stay Tune with us\n");
                showScore(score); // Display the final score
                mainMenu(); // Return to the main menu
                running = 0; // Exit the game loop
            }
        }

        // Render the game
        renderGame();

        // Delay for frame rate control (~60 FPS)
        SDL_Delay(16);
    }
}


// Main function
int main(int argc, char *argv[]) {
    if (!initSDL()) return -1;

    mainMenu();
    
    initializeBuffs();
    initializeBullets();
    score = 100;  // Set initial score to 100
    gameLoop();  
    cleanupSDL();

    return 0;
}
