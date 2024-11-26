#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>

#define CELL_SIZE 30
#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT 300
#define ROWS WINDOW_WIDTH / CELL_SIZE
#define COLS WINDOW_HEIGHT / CELL_SIZE

typedef struct cell
{
	char id[3];

	// have a pointer to the neighbouring cells
	struct cell *north;
	struct cell *south;
	struct cell *east;
	struct cell *west;

	// record whether this cell is wall or a path
	// default is a wall.
	bool wall;
	// this stops cycles - set when you travel through a cell
	bool visited;
	bool on_path;
} Cell;

void initialised_cell(Cell cells[][COLS])
{
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            sprintf(cells[r][c].id, "%d%d", r, c);
            cells[r][c].north = (r > 0) ? &cells[r - 1][c] : NULL;
            cells[r][c].south = (r < ROWS - 1) ? &cells[r + 1][c] : NULL;
            cells[r][c].east = (c < COLS - 1) ? &cells[r][c + 1] : NULL;
            cells[r][c].west = (c > 0) ? &cells[r][c - 1] : NULL;

            cells[r][c].wall = true;
            cells[r][c].visited = false;
            cells[r][c].on_path = false;
        }
    }
}


void draw_cells(SDL_Renderer *renderer, Cell cells[][COLS])
{
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            SDL_Rect rect = {c * CELL_SIZE, r * CELL_SIZE, CELL_SIZE, CELL_SIZE};

            if (cells[r][c].on_path) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for path found
                SDL_RenderFillRect(renderer, &rect);
            } else if (!cells[r][c].wall) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow for path
                SDL_RenderFillRect(renderer, &rect);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green for walls
                SDL_RenderDrawRect(renderer, &rect);
            }
        }
    }
}

void toggle_wall(int mouse_x, int mouse_y, Cell cells[][COLS])
{
	int row = mouse_y / CELL_SIZE;
    int col = mouse_x / CELL_SIZE;

    cells[row][col].wall = !cells[row][col].wall;
}


/**
 * start a recursive search for paths from the selected cell
 * it searches till it hits the East 'wall' then stops
 * it does not necessarily find the shortest path
 * 
 */

bool search(Cell *current) {
    if (current == NULL || current->wall || current->visited) {
        return false;
    }

    current->visited = true;
    current->on_path = true;

    // Check if this cell is in the rightmost column
    if (current->east == NULL) {
        return true;
    }

    // Recursively search in each direction
    if (search(current->east) || search(current->south) || search(current->north) || search(current->west)) {
        return true;
    }

    // No path found from this cell, reset the on_path flag
    current->on_path = false;
    return false;
}

void find_path(Cell *target_cell, Cell cells[][COLS]) {
    // Reset visited and on_path flags before each search
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            cells[r][c].visited = false;
            cells[r][c].on_path = false;
        }
    }

    // Start recursive search from the target cell
    search(target_cell);
}

int main(int argc, char *args[])
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window *window = SDL_CreateWindow("Maze", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 300, 300, 0);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

	// Initialised cells
	Cell cells[ROWS][COLS];
	initialised_cell(cells);
	

	// Application state
	bool isRunning = true;
	SDL_Event event;

	while (isRunning) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    isRunning = false;
                    break;

                case SDL_MOUSEBUTTONDOWN: {
                    int mouse_x = event.motion.x;
                    int mouse_y = event.motion.y;

                    if (event.button.button == SDL_BUTTON_LEFT) {
                        toggle_wall(mouse_x, mouse_y, cells);
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        int row = mouse_y / CELL_SIZE;
                        int col = mouse_x / CELL_SIZE;

                        if (!cells[row][col].wall && col == 0) {
                            find_path(&cells[row][col], cells);
                        }
                    }
                    break;
                }
            }
        }

		// Draw black background
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		SDL_RenderClear(renderer);

		// Draw Cells
		draw_cells(renderer, cells);

		// Present Render to screen
		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
