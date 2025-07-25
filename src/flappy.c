#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define MS_PER_SECOND 1000.0f

#define FLAP_CEILING 70.0f
#define GROUND 440.0f
#define PLAYER_STARTING_X 160.0f
#define PLAYER_STARTING_Y 180.0f

/*
 * # How I determined velocity and gravity
 *
 * Looking at videos of flappy bird, the bird barely goes up its height before
 * starting to fall. And trying to time from flap to returning to its starting
 * height, it's basically half a second.
 *
 * So the bird follows an inverse parabola (-x^2)
 * If I want the bird to go up its height, 40 px, I add it (-x^2 + 40)
 * Then to figure out how to have the parabola reach 0 in 0.5 seconds, I needed
 * to figure out this equation: -n(0.25)^2 + 40 = 0
 * The answer is this: n = -640
 * Giving us this: -640x^2 + 40
 * Then, to shift the inverse parabola so that 0 seconds is a height of 0, and
 * 0.5 seconds is a height of 0, I subtract 0.25 from x before squaring.
 * We end up with this: -640(x-0.25)^2 + 40
 *
 * But to figure out what acceleration we need to achieve this curve with just
 * acceleration (gravity) and a starting velocity, we need to do some calculus.
 *
 * First, let's clean up the starting equation:
 * -640(x-0.25)^2 + 40 = y
 * -640(x^2 - 0.5x + 0.0625) + 40 = y
 * -640x^2 + 320x -40 + 40 = y
 * -640x^2 + 320x = y
 *
 * When we plug in 0 and 0.5 for x, we still get 0, and 0.25 for x gets us 40
 *
 * Now we get the first derivitive, which we'll use to calculate our starting
 * flap velocity
 * -640x^2 + 320x = y
 * 2 * -640x + 320 = y
 * -1280x + 320 = y
 *
 * Plugging in 0, we get a starting velocity of 320 (pixels/second)
 *
 * Now we need the second derivitive to reckon the acceleration
 * -1280x^1 + 320x^0 = y
 * 1*-1280 + 0*320 = y
 * -1280 = y
 *
 * So that leaves us with an acceleration (gravity) of -1280 (pixels/second^2)
 *
 * Finally, since the top of the window is 0 and going down increases y, we
 * inverse the numbers, giving our final values:
 *
 * Flap velocity: -320
 * Gavity: 1280
 *
 * And when I plugged in those numbers and ran the game, it felt right!
 */
#define FLAP_VELOCITY -320.0f
#define GRAVITY 1280.0f

#define PIPE_VELOCITY 100.0f

#define PLAYER_WIDTH 40.0f
#define PIPE_WIDTH 60.0f
#define PIPE_GAP 120.0f
#define PIPE_Y_MIN 140.0f
#define PIPE_Y_MAX 370.0f

#define GAME_OVER_TIME ((uint64_t) 2000)

static uint64_t gameOverStart = 0;

// Waiting for user to click screen to start
#define GAME_STATE_START 0
// In the game loop
#define GAME_STATE_PLAY 1
// Game over, buffer time so you see the end of the game before restarting
#define GAME_STATE_OVER 2
// Game over, click button to restart
#define GAME_STATE_END 3

static uint8_t gameState = GAME_STATE_PLAY;

struct PlayerData {
    float x;
    float y;
    float v;
};

static SDL_Window *window;
static SDL_Renderer *renderer;

static uint8_t isKeyDown = 0;
static uint64_t prevTick = 0;

static struct PlayerData playerPos = {
    .x = PLAYER_STARTING_X,
    .y = PLAYER_STARTING_Y,
    .v = FLAP_VELOCITY,
};

static int pipe_current = 0;
static int pipe_next = 0;
static SDL_FPoint pipes[4];
static uint8_t stop = 0;

void resetPipes()
{
    pipe_current = 0;
    pipe_next = 0;
    for (int i = 0; i < 4; i++) {
        pipes[i].x = -3.0f * PIPE_WIDTH;
    }
}

void newPipe()
{
    float window = PIPE_Y_MAX - PIPE_Y_MIN;
    pipes[pipe_next].x = WINDOW_WIDTH + PIPE_WIDTH;
    pipes[pipe_next].y = PIPE_Y_MIN + SDL_randf() * window;

    pipe_current = pipe_next;
    pipe_next += 1;
    if (pipe_next >= 4) {
        pipe_next = 0;
    }
}

void resetPlayerData()
{
    playerPos.x = PLAYER_STARTING_X;
    playerPos.y = PLAYER_STARTING_Y;
    playerPos.v = FLAP_VELOCITY;
}

void resetGame()
{
    resetPlayerData();
    prevTick = SDL_GetTicks();
    resetPipes();
    newPipe();
    gameState = GAME_STATE_PLAY;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Flappy Bird Project", "1.0.0", "net.faisonz.games.flappy");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to init SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Flappy Bird", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Failed to create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    resetGame();

    return SDL_APP_CONTINUE;
}

SDL_AppResult processEventPlay(SDL_Event *event)
{
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_SPACE && isKeyDown == 0) {
            if (playerPos.y > FLAP_CEILING) {
                playerPos.v = FLAP_VELOCITY;
                isKeyDown = 1;
            }
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult processEventEnd(SDL_Event *event)
{
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_SPACE && isKeyDown == 0) {
            resetGame();
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    } else if (event->type == SDL_EVENT_KEY_UP) {
        if (event->key.key == SDLK_SPACE) {
            isKeyDown = 0;
        }
    }


    switch (gameState) {
        case GAME_STATE_PLAY:
            return processEventPlay(event);
            break;
        case GAME_STATE_END:
            return processEventEnd(event);
            break;
    }

    return SDL_APP_CONTINUE;
}

void tickPlay(float delta, uint64_t now)
{
    if (pipes[pipe_current].x <= ((float) WINDOW_WIDTH) * 2.0f / 3.0f) {
        newPipe();
    }

    // Update player position
    playerPos.y += playerPos.v / MS_PER_SECOND * delta;
    playerPos.v += GRAVITY / MS_PER_SECOND * delta;

    if (playerPos.y > GROUND) {
       playerPos.y = GROUND;
    }

    // Update pipe position
    for (int i = 0; i < 4; i++) {
        pipes[i].x -= PIPE_VELOCITY / MS_PER_SECOND * delta;
    }

    // Check game over condition
    if (playerPos.y >= (float) GROUND - 0.25f * PLAYER_WIDTH) {
        gameState = GAME_STATE_OVER;
        gameOverStart = now;
    }
}

void tickOver(float delta, uint64_t now)
{
    if (gameOverStart + GAME_OVER_TIME <= now) {
        gameState = GAME_STATE_END;
    }
}

void tick()
{
    // TICK UPDATE
    const uint64_t now = SDL_GetTicks();
    float delta = (float) (now - prevTick);

    switch (gameState) {
        case GAME_STATE_PLAY:
            tickPlay(delta, now);
            break;
        case GAME_STATE_OVER:
            tickOver(delta, now);
            break;
    }

    prevTick = now;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{

    tick();

    // RENDER
    SDL_SetRenderDrawColor(renderer, 0, 153, 219, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // Cloud line
    SDL_SetRenderDrawColor(renderer, 255, 238, 229, SDL_ALPHA_OPAQUE);
    SDL_RenderLine(renderer, 0, FLAP_CEILING, WINDOW_WIDTH, FLAP_CEILING);

    // Ground
    SDL_FRect ground = {
        .x = 0,
        .y = GROUND,
        .w = WINDOW_WIDTH,
        .h = WINDOW_HEIGHT - GROUND,
    };
    SDL_SetRenderDrawColor(renderer, 67, 189, 53, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &ground);

    // Pipes
    SDL_SetRenderDrawColor(renderer, 167, 255, 25, SDL_ALPHA_OPAQUE);
    SDL_FRect pipeRects[8];

    for (int i = 0; i < 4; i++) {
        int topI = 2 * i;
        int botI = 2 * i + 1;
        pipeRects[topI].x = pipes[i].x - PIPE_WIDTH / 2.0f;
        pipeRects[topI].y = 0;
        pipeRects[topI].w = PIPE_WIDTH;
        pipeRects[topI].h = pipes[i].y - PIPE_GAP / 2.0f;

        pipeRects[botI].x = pipeRects[topI].x;
        pipeRects[botI].y = pipeRects[topI].h + PIPE_GAP;
        pipeRects[botI].w = PIPE_WIDTH;
        pipeRects[botI].h = GROUND - pipeRects[botI].y;
    }

    SDL_RenderFillRects(renderer, pipeRects, 8);

    // Bird
    SDL_FRect player = {
        .x = playerPos.x - PLAYER_WIDTH / 2.0f,
        .y = playerPos.y - PLAYER_WIDTH / 2.0f,
        .w = PLAYER_WIDTH,
        .h = PLAYER_WIDTH,
    };

    SDL_SetRenderDrawColor(renderer, 254, 231, 97, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &player);

    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_DestroyRenderer(renderer);
    renderer = NULL;

    SDL_DestroyWindow(window);
    window = NULL;
}
