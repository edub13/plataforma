#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"

#include <stdio.h>
#define WINDOW_TITLE "Title" // Titulo
#define WINDOW_WIDTH 640 // Largura da janela
#define WINDOW_HEIGHT 480 // Altura da janela

#define PLAYER_WIDTH 20 // Largura do player
#define PLAYER_HEIGHT 48 // Altura do player
#define PLAYER_HVELOCITY 5 // Velocidade player
#define PLAYER_GRAVITY 2 // Valor da gravidade
#define PLAYER_JUMP_VELOCITY 20 // Velocidade do salto
#define PLAYER_TERMINAL_VELOCITY 20 // Velocidade final

#define GROUND_WIDTH 60 // Largura do ground
#define GROUND_HEIGHT 60 // Altura do ground
#define GROUND_COUNT ((WINDOW_WIDTH / GROUND_WIDTH) + (WINDOW_WIDTH % GROUND_WIDTH != 0)) // Contador do ground

typedef struct { // struct App
    SDL_Window* window; // Para criar janela
    SDL_Renderer* renderer; // Para criar renderizador
    SDL_Event event; // Para os eventos (Teclado, rato, etc)
    int quit; // Para o ciclo do jogo (0 = falso (continuar a correr), 1 = verdadeiro (o ciclo termina))

    struct { // struct resources
        struct { // struct images
            SDL_Texture* platform;
            SDL_Texture* player;
            SDL_Texture* ground;
        } images;
    } resources;

    struct { // struct player
        SDL_Texture** texture;
        SDL_Rect box;
        SDL_Point vel;
        int jumping;
    } player;

    struct { // struct ground (para o "chão" do jogo)
        SDL_Texture** texture;
        SDL_Rect* box;
        unsigned int count;
    } ground;

    struct { // struct platform (para as plataformas para onde o player saltará)
        SDL_Texture** texture;
        SDL_Rect* box;
        unsigned int count;
    } platform;
} App;

/* Funções */

int Init(App* app); // Iniciar SDL, carregar imagens, verificação de erros.
int Loop(App* app); // Ciclo do jogo
int Update(App* app); // Actualizar a "janela"
int Draw(App* app); // Desenhar "coisas" para a janela
int Exit(App* app); // Apagar imagens da memória, destruir janela, renderizador e desligar SDL

int isColliding(SDL_Rect* a, SDL_Rect* b); // Verificar colisão entre dois SDL_Rect (Se colidir retorna 1, se não retorna 0)
int WColliding(SDL_Rect* a); // Verificar colisão da entre objectos e as bordas da janela

SDL_Texture* LoadIMG(const char* path, SDL_Renderer* renderer) // Carregar imagens (exemplo: SDL_Texture* inimigo = LoadIMG("inimigo.png", renderer))
{
    SDL_Surface* surface = IMG_Load(path);
    if(!surface) {
        printf("Error loading image %s : %s\n", path, IMG_GetError());
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return texture;
}

int Init(App* app) { // Inicializar SDL
    if(SDL_Init(SDL_INIT_VIDEO) != 0) // Verificar se a SDL inicializou
        return 1;
	// Criar Janela
    app->window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if(!app->window) { // Verificar se a janela foi criada
        printf("Error creating window : %s", SDL_GetError());
        return 2;
    }
	// Criar renderizador
    app->renderer = SDL_CreateRenderer(app->window, -1, 0);
    if(!app->renderer) {
        printf("Error creating window renderer : %s\n", SDL_GetError());
        return 3;
    }
	// Carregar imagens
    app->resources.images.player   = LoadIMG("player.png",     app->renderer);
    app->resources.images.ground   = LoadIMG("ground.png",     app->renderer);
    app->resources.images.platform = LoadIMG("platform.png",   app->renderer);
	// Passa a imagem da struct resources.images para a struct player (acho que dá para perceber).
    app->player.texture = &app->resources.images.player;
	// "Rectangulo" em volta da imagem do player
    app->player.box.x = (WINDOW_WIDTH + PLAYER_WIDTH) / 2; // Posição do retangulo do jogador no eixo de X
    app->player.box.y = WINDOW_HEIGHT - GROUND_HEIGHT - PLAYER_HEIGHT; // Posição do retangulo do jogador no eixo de Y
    app->player.box.w = PLAYER_WIDTH; // Largura do retangulo do jogador
    app->player.box.h = PLAYER_HEIGHT; // Altura do retangulo do jogador
    app->player.vel.x = 0; // Velocidade do jogador no eixo do X
    app->player.vel.y = 0; // Velocidade do jogador no eixo do Y
    app->player.jumping = 0; // "Velocidade" do salto do jogador 
	// Passa a imagem da struct resources para a ground
    app->ground.texture = &app->resources.images.ground;
	// Contador do "chão"
    app->ground.count = GROUND_COUNT;
	// Alocar memória para o "retangulo" do "chão" (visto que não será desenhado um a um ?)
    app->ground.box = malloc(sizeof(SDL_Rect) * app->ground.count);
    for (int i = 0; i < app->ground.count; ++i) {
        app->ground.box[i].x = 0 + (i * GROUND_WIDTH);
        app->ground.box[i].y = WINDOW_HEIGHT - GROUND_HEIGHT;
        app->ground.box[i].w = GROUND_WIDTH;
        app->ground.box[i].h = GROUND_HEIGHT;
    }

    app->platform.texture = &app->resources.images.platform;
    app->platform.box = NULL;
    app->platform.count = 0;

    return 0;
}
// Ciclo do jogo
int Loop(App* app) {
    app->quit = 0;
    while(!app->quit) {
        while(SDL_PollEvent(&app->event)) {
            switch(app->event.type) {
                case SDL_QUIT:
                    app->quit = 1; // Sair
                    break;
                case SDL_KEYDOWN: // Tecla premida
                    switch (app->event.key.keysym.sym) {
                        case SDLK_LEFT:
                            app->player.vel.x = -PLAYER_HVELOCITY;
                            break;
                        case SDLK_RIGHT:
                            app->player.vel.x = PLAYER_HVELOCITY;
                            break;
                        case SDLK_UP: 
                            if (!app->player.jumping) {
                                app->player.jumping = 1;
                                app->player.vel.y = -PLAYER_JUMP_VELOCITY;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_KEYUP:// Tecla "desprimida"
                    switch (app->event.key.keysym.sym) {
                        case SDLK_LEFT:
                        case SDLK_RIGHT:
                            app->player.vel.x = 0;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }

        Update(app);
        Draw(app);

        SDL_Delay(30); // always do a delay ... stop melting the CPU
    }

    return 0;
}

int Update(App* app) {
    app->player.vel.y += 2;
    if (app->player.vel.y > PLAYER_TERMINAL_VELOCITY)
        app->player.vel.y = PLAYER_TERMINAL_VELOCITY;

    app->player.box.x += app->player.vel.x;
    app->player.box.y += app->player.vel.y;

    if(WColliding(&app->player.box))
        app->player.vel.y = 0;

    for (int i = 0; i < app->ground.count; ++i) {
        if (isColliding(&app->player.box, &app->ground.box[i])) {
            app->player.jumping = 0;
            app->player.box.y = app->ground.box[i].y - PLAYER_HEIGHT;
        }
    }

    for (int i = 0; i < app->platform.count; ++i) {
        if (isColliding(&app->player.box, &app->platform.box[i])) {
            // TODO : action
        }
    }

    return 0;
}

int Draw(App* app){
    SDL_RenderClear(app->renderer);

    SDL_SetRenderDrawColor(app->renderer, 100, 149, 237, 0); // cornflower blue
    SDL_RenderClear(app->renderer);

    for (int i = 0; i < app->ground.count; ++i)
        SDL_RenderCopy(app->renderer, *app->ground.texture, NULL, &app->ground.box[i]);

    for (int i = 0; i < app->platform.count; ++i)
        SDL_RenderCopy(app->renderer, *app->platform.texture, NULL, &app->platform.box[i]);

    SDL_RenderCopy(app->renderer, *app->player.texture, NULL, &app->player.box);

    SDL_RenderPresent(app->renderer);

    return 0;
}

int isColliding(SDL_Rect* a, SDL_Rect* b)
{
    if (   a->x < b->x + b->w
        && a->x + a->w > b->x
        && a->y < b->y + b->h
        && a->h + a->y > b->y){
        return 1;
    }
    return 0;
}

int WColliding(SDL_Rect* a)
{
    if(a->x < 0)
        a->x = 0;

    if(a->y < 0)
        a->y = 0;

    if((a->x + a->w) > WINDOW_WIDTH)
        a->x = WINDOW_WIDTH - a->w;

    if((a->y + a->h) > WINDOW_HEIGHT)
        a->y = WINDOW_HEIGHT - a->h;

    return 0;
}

int Exit(App* app) {
    SDL_DestroyTexture(app->resources.images.platform);
    SDL_DestroyTexture(app->resources.images.ground);
    SDL_DestroyTexture(app->resources.images.player);

    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);

    SDL_Quit();

    if (app->ground.box)
        free(app->ground.box);
    if (app->platform.box)
        free(app->platform.box);

    return 0;
}

int main(int argc, char ** argv) {
    App app;

    if (Init(&app) == 0) {
        Loop(&app);
        Exit(&app);
    } else
        printf("Error : %s\n", SDL_GetError());

    return 0;
}
