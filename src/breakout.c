#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL2/SDL2_gfxPrimitives.h> // For drawing filled circles

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define PADDLE_WIDTH 100
#define PADDLE_HEIGHT 20

#define BALL_RADIUS 10

#define BRICK_ROWS 5
#define BRICK_COLUMNS 10
#define BRICK_WIDTH 60
#define BRICK_HEIGHT 20

typedef struct {
    float x, y;
    float width, height;
    float speed;
} Paddle;

typedef struct {
    float x, y;
    float radius;
    float speedX, speedY;
} Ball;

typedef struct {
    float x, y;
    float width, height;
    int isVisible;
} Brick;

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create Window
    SDL_Window *win = SDL_CreateWindow("Breakout Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create Renderer
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
                                           SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == NULL){
        SDL_DestroyWindow(win);
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Game objects
    Paddle paddle = {0};
    Ball ball = {0};
    // Allocate bricks on the heap
    Brick *bricks = malloc(sizeof(Brick) * BRICK_ROWS * BRICK_COLUMNS);
    if (!bricks) {
        printf("Failed to allocate memory for bricks.\n");
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    // Initialize Paddle
    paddle.width = PADDLE_WIDTH;
    paddle.height = PADDLE_HEIGHT;
    paddle.x = (WINDOW_WIDTH - paddle.width) / 2;
    paddle.y = WINDOW_HEIGHT - paddle.height - 10;
    paddle.speed = 400.0f; // pixels per second

    // Initialize Ball
    ball.radius = BALL_RADIUS;
    ball.x = WINDOW_WIDTH / 2;
    ball.y = WINDOW_HEIGHT / 2;
    ball.speedX = 200.0f;
    ball.speedY = -200.0f;

    // Initialize Bricks
    for (int i = 0; i < BRICK_ROWS; ++i) {
        for (int j = 0; j < BRICK_COLUMNS; ++j) {
            Brick *brick = &bricks[i * BRICK_COLUMNS + j];
            brick->width = BRICK_WIDTH;
            brick->height = BRICK_HEIGHT;
            brick->x = j * (BRICK_WIDTH + 10) + 35;
            brick->y = i * (BRICK_HEIGHT + 5) + 50;
            brick->isVisible = 1;
        }
    }

    // Difficulty selection
    float difficultyMultiplier = 1.0f;

    // Check for command-line argument
    if (argc > 1) {
        int difficulty = atoi(argv[1]);
        switch (difficulty) {
            case 1: difficultyMultiplier = 0.8f; break; // Easy
            case 2: difficultyMultiplier = 1.0f; break; // Medium
            case 3: difficultyMultiplier = 1.2f; break; // Hard
            default: difficultyMultiplier = 1.0f; break; // Default to Medium
        }
    } else {
        // Set default difficulty to Medium
        difficultyMultiplier = 1.0f;
    }

    // Adjust speeds
    paddle.speed *= difficultyMultiplier;
    ball.speedX *= difficultyMultiplier;
    ball.speedY *= difficultyMultiplier;

    // Main loop flag
    int quit = 0;
    SDL_Event e;

    // Timing variables
    Uint32 lastTick = SDL_GetTicks();

    // Frame rate control
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;
    Uint32 frameStart;
    int frameTime;

    // Main loop
    while (!quit) {
        frameStart = SDL_GetTicks();

        // Calculate deltaTime
        Uint32 currentTick = SDL_GetTicks();
        float deltaTime = (currentTick - lastTick) / 1000.0f;
        lastTick = currentTick;

        // Event handler
        while (SDL_PollEvent(&e) != 0) {
            // User requests quit
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }

        // Handle input
        const Uint8 *state = SDL_GetKeyboardState(NULL);

        if (state[SDL_SCANCODE_LEFT]) {
            paddle.x -= paddle.speed * deltaTime;
            if (paddle.x < 0) paddle.x = 0;
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            paddle.x += paddle.speed * deltaTime;
            if (paddle.x > WINDOW_WIDTH - paddle.width) paddle.x = WINDOW_WIDTH - paddle.width;
        }

        // Update ball position
        ball.x += ball.speedX * deltaTime;
        ball.y += ball.speedY * deltaTime;

        // Collision with walls
        if (ball.x - ball.radius < 0) {
            ball.speedX = -ball.speedX;
            ball.x = ball.radius;
        }
        if (ball.x + ball.radius > WINDOW_WIDTH) {
            ball.speedX = -ball.speedX;
            ball.x = WINDOW_WIDTH - ball.radius;
        }
        if (ball.y - ball.radius < 0) {
            ball.speedY = -ball.speedY;
            ball.y = ball.radius;
        }
        if (ball.y - ball.radius > WINDOW_HEIGHT) {
            // Ball is lost; reset position
            ball.x = WINDOW_WIDTH / 2;
            ball.y = WINDOW_HEIGHT / 2;
            // Optionally, decrease lives or end game
        }

        // Collision with paddle
        if (ball.y + ball.radius >= paddle.y &&
            ball.x >= paddle.x &&
            ball.x <= paddle.x + paddle.width &&
            ball.speedY > 0) {
            ball.speedY = -ball.speedY;
            ball.y = paddle.y - ball.radius; // Adjust position to avoid sticking
        }

        // Collision with bricks
        for (int i = 0; i < BRICK_ROWS * BRICK_COLUMNS; ++i) {
            Brick *brick = &bricks[i];
            if (brick->isVisible &&
                ball.x + ball.radius >= brick->x &&
                ball.x - ball.radius <= brick->x + brick->width &&
                ball.y + ball.radius >= brick->y &&
                ball.y - ball.radius <= brick->y + brick->height) {

                brick->isVisible = 0;

                // Simple collision response
                ball.speedY = -ball.speedY;

                break;
            }
        }

        // Clear screen
        if (SDL_SetRenderDrawColor(ren, 0, 0, 0, 255) != 0) {
            printf("SDL_SetRenderDrawColor Error: %s\n", SDL_GetError());
        }
        if (SDL_RenderClear(ren) != 0) {
            printf("SDL_RenderClear Error: %s\n", SDL_GetError());
        }

        // Draw Paddle
        SDL_Rect paddleRect = { (int)paddle.x, (int)paddle.y, (int)paddle.width, (int)paddle.height };
        if (SDL_SetRenderDrawColor(ren, 255, 255, 255, 255) != 0) {
            printf("SDL_SetRenderDrawColor Error: %s\n", SDL_GetError());
        }
        if (SDL_RenderFillRect(ren, &paddleRect) != 0) {
            printf("SDL_RenderFillRect Error: %s\n", SDL_GetError());
        }

        // Draw Ball
        // Use SDL2_gfx for filled circle
        filledCircleRGBA(ren, (int16_t)ball.x, (int16_t)ball.y, (int16_t)ball.radius, 255, 255, 255, 255);

        // Draw Bricks
        for (int i = 0; i < BRICK_ROWS * BRICK_COLUMNS; ++i) {
            Brick *brick = &bricks[i];
            if (brick->isVisible) {
                SDL_Rect brickRect = { (int)brick->x, (int)brick->y, (int)brick->width, (int)brick->height };
                // Set color based on row for variety
                switch (i / BRICK_COLUMNS % 5) {
                    case 0: SDL_SetRenderDrawColor(ren, 255, 0, 0, 255); break;     // Red
                    case 1: SDL_SetRenderDrawColor(ren, 0, 255, 0, 255); break;     // Green
                    case 2: SDL_SetRenderDrawColor(ren, 0, 0, 255, 255); break;     // Blue
                    case 3: SDL_SetRenderDrawColor(ren, 255, 255, 0, 255); break;   // Yellow
                    case 4: SDL_SetRenderDrawColor(ren, 255, 165, 0, 255); break;   // Orange
                }
                if (SDL_RenderFillRect(ren, &brickRect) != 0) {
                    printf("SDL_RenderFillRect Error: %s\n", SDL_GetError());
                }
            }
        }

        // Update screen
        SDL_RenderPresent(ren);

        // Frame rate control
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    // Clean up
    free(bricks);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

