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
#define FLAP_VELOCITY -700.0f
#define GRAVITY 2000.0f
#define PIPE_VELOCITY 100.0f

#define PLAYER_WIDTH 40.0f
#define PIPE_WIDTH 60.0f
#define PIPE_GAP 140.0f
#define PIPE_Y_MIN 140.0f
#define PIPE_Y_MAX 370.0f

// Waiting for user to click screen to start
#define GAME_STATE_START 0
// In the game loop
#define GAME_STATE_PLAY 1
// Game over, click button to restart
#define GAME_STATE_END 2

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

    resetPlayerData();
    prevTick = SDL_GetTicks();
    resetPipes();
    newPipe();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        SDL_KeyboardEvent *e = (SDL_KeyboardEvent*) event;
        if (e->key == SDLK_SPACE && isKeyDown == 0) {
            if (playerPos.y > FLAP_CEILING) {
                playerPos.v = FLAP_VELOCITY;
                isKeyDown = 1;
            }
        }
    } else if (event->type == SDL_EVENT_KEY_UP) {
        SDL_KeyboardEvent *e = (SDL_KeyboardEvent*) event;
        if (e->key == SDLK_SPACE) {
            isKeyDown = 0;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{

    // TICK UPDATE
    const uint64_t now = SDL_GetTicks();
    float delta = (float) (now - prevTick);

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

    prevTick = now;

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
