#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include "input_functions.h"

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 700
#define FONT_SIZE 25

#define MARGIN_LEFT 50
#define ALBUM_SPACE 300
#define TRACK_SPACING 50
#define COVER_SIZE 200
#define TRACK_LIST_X 300
#define COVER_Y_OFFSET 50

#define AUDIO_FREQUENCY 44100
#define AUDIO_CHUNK_SIZE 2048

#define MAX_PATH_LENGTH 256

const SDL_Color COLOR_WHITE = {255, 255, 255};
const SDL_Color COLOR_RED = {255, 0, 0};

typedef enum genre {
    POP = 1,
    CLASSIC = 2,
    JAZZ = 3,
    ROCK = 4
} Genre;

typedef struct track {
    Mix_Music *music;
    char title[256];
    char location[MAX_PATH_LENGTH];
} Track;

void trim_newline(char *str) {
    char *newline = strpbrk(str, "\r\n");
    if (newline) {
        *newline = '\0';
    }
}

typedef struct album {
    SDL_Texture *cover;
    char title[MAX_PATH_LENGTH];
    char artist[MAX_PATH_LENGTH];
    char photo_location[MAX_PATH_LENGTH];
    Genre genre;
    int number_of_tracks;
    Track *tracks;
} Album;

Track read_track(FILE *fptr) {
    Track track = {0};
    if (!fgets(track.title, sizeof(track.title), fptr) || !fgets(track.location, sizeof(track.location), fptr)) {
        printf("Error reading track information\n");
        return track;
    }
    trim_newline(track.title);
    trim_newline(track.location);

    printf("Loading music: %s\n", track.location);
    track.music = Mix_LoadMUS(track.location);
    if (!track.music) {
        printf("Error loading music: %s - %s\n", track.location, Mix_GetError());
    }

    return track;
}

Track *read_tracks(FILE *fptr, int number_of_tracks) {
    Track *tracks = malloc(number_of_tracks * sizeof(Track));
    if (!tracks) {
        printf("Memory allocation failed for tracks\n");
        return NULL;
    }

    for (int i = 0; i < number_of_tracks; i++) {
        tracks[i] = read_track(fptr);
    }

    return tracks;
}

Album read_album(FILE *fptr, SDL_Renderer *renderer) {
    Album album = {0};

    if (!fgets(album.title, sizeof(album.title), fptr) ||
        !fgets(album.artist, sizeof(album.artist), fptr) ||
        !fgets(album.photo_location, sizeof(album.photo_location), fptr)) {
        printf("Error reading album information\n");
        return album;
    }
    trim_newline(album.title);
    trim_newline(album.artist);
    trim_newline(album.photo_location);

    SDL_Surface *surface = IMG_Load(album.photo_location);
    if (!surface) {
        printf("Error loading image: %s\n", album.photo_location, IMG_GetError());
    } else {
        album.cover = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    int genre_code;
    if (fscanf(fptr, "%d\n", &genre_code) != 1) {
        printf("Error reading genre information\n");
        return album;
    }
    album.genre = (Genre)genre_code;

    if (fscanf(fptr, "%d\n", &album.number_of_tracks) != 1) {
        printf("Error reading number of tracks\n");
        return album;
    }

    album.tracks = read_tracks(fptr, album.number_of_tracks);

    return album;
}

Album *read_albums(FILE *fptr, int *number_of_albums, SDL_Renderer *renderer) {
    if (fscanf(fptr, "%d\n", number_of_albums) != 1) {
        printf("Error reading number of albums\n");
        return NULL;
    }

    Album *albums = malloc(*number_of_albums * sizeof(Album));
    if (!albums) {
        printf("Memory allocation failed for albums\n");
        return NULL;
    }

    for (int i = 0; i < *number_of_albums; i++) {
        albums[i] = read_album(fptr, renderer);
        if (!albums[i].cover) {
            printf("Error loading album cover for album %d\n", i + 1);
        }
    }

    return albums;
}

bool is_point_in_rect(int x, int y, SDL_Rect rect) {
    return (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h);
}

void handle_click(int mouse_x, int mouse_y, Album *albums, int number_of_albums, Mix_Music **current_music, int *current_album, int *current_track) {
    for (int a = 0; a < number_of_albums; a++) {
        SDL_Rect album_rect = {MARGIN_LEFT, COVER_Y_OFFSET + a * ALBUM_SPACE, COVER_SIZE, COVER_SIZE};

        if (mouse_x >= album_rect.x && mouse_x <= album_rect.x + album_rect.w &&
            mouse_y >= album_rect.y && mouse_y <= album_rect.y + album_rect.h) {
            printf("Album %d clicked\n", a + 1);
            
            // Update current album and reset current track
            *current_album = a;
            *current_track = -1;

            // Stop any currently playing music
            if (*current_music) {
                Mix_FreeMusic(*current_music);
                *current_music = NULL;
            }

            return;
        }

        // Check track clicks within the album
        for (int t = 0; t < albums[a].number_of_tracks; t++) {
            SDL_Rect track_rect = {TRACK_LIST_X, COVER_Y_OFFSET + t * TRACK_SPACING + a * ALBUM_SPACE, 240, FONT_SIZE};

            if (mouse_x >= track_rect.x && mouse_x <= track_rect.x + track_rect.w &&
                mouse_y >= track_rect.y && mouse_y <= track_rect.y + track_rect.h) {
                printf("Track %d in Album %d clicked\n", t + 1, a + 1);

                // Update current track
                *current_album = a;
                *current_track = t;

                // Stop any currently playing music
                if (Mix_PlayingMusic()) {
                    Mix_HaltMusic();
                }

                // Play the selected track
                *current_music = albums[a].tracks[t].music;
                Mix_PlayMusic(*current_music, -1);
                return;
            }
        }
    }
}



void draw_single_album(Album *album, SDL_Renderer *renderer, TTF_Font *font, int y_offset, int album_index, int current_album, int current_track) {
    SDL_Color title_color = (album_index == current_album) ? COLOR_RED : COLOR_WHITE;
    SDL_Surface *title_surface = TTF_RenderText_Blended(font, album->title, title_color);
    if (title_surface) {
        SDL_Texture *title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
        SDL_Rect title_rect = {MARGIN_LEFT, y_offset, title_surface->w, title_surface->h};

        SDL_RenderCopy(renderer, title_texture, NULL, &title_rect);
        SDL_FreeSurface(title_surface);
        SDL_DestroyTexture(title_texture);
    }

    SDL_Surface *artist_surface = TTF_RenderText_Blended(font, album->artist, COLOR_WHITE);
    if (artist_surface) {
        SDL_Texture *artist_texture = SDL_CreateTextureFromSurface(renderer, artist_surface);
        SDL_Rect artist_rect = {MARGIN_LEFT, y_offset + FONT_SIZE, artist_surface->w, artist_surface->h};

        SDL_RenderCopy(renderer, artist_texture, NULL, &artist_rect);
        SDL_FreeSurface(artist_surface);
        SDL_DestroyTexture(artist_texture);
    }

    if (album->cover) {
        SDL_Rect cover_rect = {MARGIN_LEFT, y_offset + COVER_Y_OFFSET, COVER_SIZE, COVER_SIZE};
        SDL_RenderCopy(renderer, album->cover, NULL, &cover_rect);
    }

    if (album_index == current_album) {
        for (int i = 0; i < album->number_of_tracks; i++) {
            SDL_Color track_color = (i == current_track) ? COLOR_RED : COLOR_WHITE;
            char track_text[300];
            sprintf(track_text, "%d. %s", i + 1, album->tracks[i].title);
            SDL_Surface *track_surface = TTF_RenderText_Blended(font, track_text, track_color);
            if (track_surface) {
                SDL_Texture *track_texture = SDL_CreateTextureFromSurface(renderer, track_surface);
                SDL_Rect track_rect = {TRACK_LIST_X, y_offset + COVER_Y_OFFSET + i * TRACK_SPACING, track_surface->w, FONT_SIZE};

                // Draw the border around the track title
                SDL_Rect border_rect = {track_rect.x - 5, track_rect.y - 5, track_rect.w + 10, track_rect.h + 10};
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White border
                SDL_RenderDrawRect(renderer, &border_rect);

                SDL_RenderCopy(renderer, track_texture, NULL, &track_rect);
                SDL_FreeSurface(track_surface);
                SDL_DestroyTexture(track_texture);
            }
        }
    }
}



void draw_albums(SDL_Renderer *renderer, Album *albums, int number_of_albums, TTF_Font *font, int current_album, int current_track) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderClear(renderer);

    int y_offset = 0;
    for (int i = 0; i < number_of_albums; i++) {
        draw_single_album(&albums[i], renderer, font, y_offset, i, current_album, current_track);
        y_offset += ALBUM_SPACE;
    }

    SDL_RenderPresent(renderer);
}

bool initialize_sdl(SDL_Window **window, SDL_Renderer **renderer, TTF_Font **font) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL Initialization Failed: %s\n", SDL_GetError());
        return false;
    }

        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("IMG Initialization Failed: %s\n", IMG_GetError());
        SDL_Quit();
        return false;
    }

    if (Mix_OpenAudio(AUDIO_FREQUENCY, MIX_DEFAULT_FORMAT, 2, AUDIO_CHUNK_SIZE) < 0) {
        printf("SDL_Mixer Initialization Failed: %s\n", Mix_GetError());
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    if (TTF_Init() == -1) {
        printf("TTF Initialization Failed: %s\n", TTF_GetError());
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    *window = SDL_CreateWindow("Music Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!*window) {
        printf("Window Creation Failed: %s\n", SDL_GetError());
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) {
        printf("Renderer Creation Failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(*window);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    *font = TTF_OpenFont("arial.ttf", FONT_SIZE);
    if (!*font) {
        printf("Font Loading Failed: %s\n", TTF_GetError());
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    bool quit = false;
    SDL_Event event;
    int current_album = 0;
    int current_track = -1;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    TTF_Font *font = NULL;
    Mix_Music *current_music = NULL;

    // Initialize SDL and its components
    if (!initialize_sdl(&window, &renderer, &font)) {
        return 1;
    }

    // Open the file containing album data
    FILE *fptr = fopen("albums.txt", "r");
    if (!fptr) {
        printf("Error opening albums.txt\n");
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        Mix_CloseAudio();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    int number_of_albums = 0;
    Album *albums = read_albums(fptr, &number_of_albums, renderer);
    fclose(fptr);

    if (!albums) {
        printf("Error reading albums\n");
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        Mix_CloseAudio();
        Mix_Quit();
        SDL_Quit();
        return 1;
    }

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int mouse_x = event.button.x;
                        int mouse_y = event.button.y;                       
                        handle_click(mouse_x, mouse_y, albums, number_of_albums, &current_music, &current_album, &current_track);

                    }
                    break;
            }
        }

        // Render albums and tracks
        draw_albums(renderer, albums, number_of_albums, font, current_album, current_track);
    }


    // Cleanup resources
    for (int i = 0; i < number_of_albums; i++) {
        SDL_DestroyTexture(albums[i].cover);
        for (int j = 0; j < albums[i].number_of_tracks; j++) {
            Mix_FreeMusic(albums[i].tracks[j].music);
        }
        free(albums[i].tracks);
    }
    free(albums);

    //cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();

    IMG_Quit();
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();

    return 0;
}
