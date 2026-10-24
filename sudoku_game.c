// sudoku_game.c

#include "sudoku_game.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define SUDOKU_DIMENSIONS 9
#define INT_SET_SIZE (SUDOKU_DIMENSIONS + 1)
#define MINISQUARE_DIMENSIONS 3
#define SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define UNUSED 0

typedef int index;
typedef int entry;

struct int_set 
{
    bool data[INT_SET_SIZE];
};

void int_set_init(struct int_set* is)
{
    memset(is->data, 0, sizeof is->data);
}

void int_set_add(struct int_set* is, index i)
{
    is->data[i] = true;
}

bool int_set_contains(struct int_set* is, index i)
{
    return is->data[i];
}

void int_set_remove(struct int_set* is, index i)
{
    is->data[i] = false;
}

struct sudoku_state
{
    entry input_grid   [SUDOKU_DIMENSIONS][SUDOKU_DIMENSIONS];
    entry solution_grid[SUDOKU_DIMENSIONS][SUDOKU_DIMENSIONS];

    struct int_set row_sets[SUDOKU_DIMENSIONS];
    struct int_set col_sets[SUDOKU_DIMENSIONS];

    struct int_set minisquare_sets[MINISQUARE_DIMENSIONS]
                                  [MINISQUARE_DIMENSIONS];

    index minisquare_x;
    index minisquare_y;
};

static void copy_input_row(int* input_row, entry* entry_row)
{
    for (size_t x = 0; x < SUDOKU_DIMENSIONS; ++x)
    {
        entry_row[x] = input_row[x];
    }
}

bool fast_solve_sudoku(struct sudoku_state* state, int (*grid)[SUDOKU_DIMENSIONS]);

bool sudoku_state_init(struct sudoku_state* state, int (*input_grid)[SUDOKU_DIMENSIONS])
{
    for (index i = 0; i < SUDOKU_DIMENSIONS; ++i)
    {
        int_set_init(&state->row_sets[i]);
        int_set_init(&state->col_sets[i]);
    }

    for (index y = 0; y < MINISQUARE_DIMENSIONS; ++y)
    {
        for (index x = 0; x < MINISQUARE_DIMENSIONS; ++x)
        {
            int_set_init(&state->minisquare_sets[y][x]);
        }
    }

    for (index y = 0; y < SUDOKU_DIMENSIONS; ++y)
    {
        copy_input_row(input_grid[y], &state->input_grid[y][0]);
        copy_input_row(input_grid[y], &state->solution_grid[y][0]);
    }

    for (index y = 0; y < SUDOKU_DIMENSIONS; ++y)
    {
        for (index x = 0; x < SUDOKU_DIMENSIONS; ++x)
        {
            entry value = state->input_grid[y][x];

            if (value == UNUSED)
            {
                continue;
            }

            if (value < 1 || value > SUDOKU_DIMENSIONS)
            {
                return false;
            }

            struct int_set* row = &state->row_sets[y];
            struct int_set* col = &state->col_sets[x];
            struct int_set* box = 
                &state->minisquare_sets[y / MINISQUARE_DIMENSIONS]
                                       [x / MINISQUARE_DIMENSIONS];

            if (int_set_contains(row, value) ||
                int_set_contains(col, value) ||
                int_set_contains(box, value))
            {
                return false;
            }

            int_set_add(row, value);
            int_set_add(col, value);
            int_set_add(box, value);
        }
    }

    return true;
}

void sudoku_state_load_minisquare_coords(
    struct sudoku_state* state,
    index x,
    index y)
{
    state->minisquare_x = x / MINISQUARE_DIMENSIONS;
    state->minisquare_y = y / MINISQUARE_DIMENSIONS;
}

void fillWithZero(int (*grid1)[9], int (*grid2)[9], int level)
{
    if (level < 0 || level > SUDOKU_DIMENSIONS)
    {
        return;
    }

    copyMatrix(grid1, grid2);

    for (int i = 0; i < SUDOKU_DIMENSIONS; ++i)
    {
        int numbers[SUDOKU_DIMENSIONS];
        memcpy(numbers, shuffle(), sizeof numbers);

        for (int j = 0; j < level; j++)
        {
            int column = numbers[j] - 1;

            grid1[i][column] = UNUSED;

            if (!countSolutions(grid1))
            {
                grid1[i][column] = grid2[i][column];
            }
        }
    }
}

bool countSolutions(int (*grid)[9])
{
    int solveGrid1[9][9] = { 0 }, solvedGrid2[9][9] = { 0 };
    copyMatrix(solveGrid1, grid);

    ULONGLONG ta = GetTickCount64();
    solveSudoku(solveGrid1, 0, 0);
    ULONGLONG tb = GetTickCount64();

    size_t duration = (size_t)(tb - ta);

    printf("OP solution duration in %zu milliseconds.\n", duration);

    struct sudoku_state state;
    
    ta = GetTickCount64();
    bool solved = fast_solve_sudoku(&state, grid);
    tb = GetTickCount64();

    duration = (size_t)(tb - ta);

    printf("CR solution duration in %zu milliseconds.\n\n", duration);

    return solved;
}

static bool fast_solve_sudoku_impl(
    struct sudoku_state* state,
    index x,
    index y)
{
    if (x == SUDOKU_DIMENSIONS)
    {
        x = 0;
        ++y;
    }

    if (y == SUDOKU_DIMENSIONS)
    {
        return true;
    }

    if (state->input_grid[y][x] != UNUSED)
    {
        return fast_solve_sudoku_impl(state, x + 1, y);
    }

    struct int_set* row = &state->row_sets[y];
    struct int_set* col = &state->col_sets[x];
    struct int_set* box = 
        &state->minisquare_sets[y / MINISQUARE_DIMENSIONS]
                               [x / MINISQUARE_DIMENSIONS];

    for (entry value = 1; value <= SUDOKU_DIMENSIONS; ++value)
    {
        if (int_set_contains(row, value) ||
            int_set_contains(col, value) ||
            int_set_contains(box, value))
        {
            continue;
        }
        
        state->solution_grid[y][x] = value;

        int_set_add(row, value);
        int_set_add(col, value);
        int_set_add(box, value);

        if (fast_solve_sudoku_impl(state, x + 1, y))
        {
            return true;
        }

        int_set_remove(row, value);
        int_set_remove(col, value);
        int_set_remove(box, value);
        
        state->solution_grid[y][x] = UNUSED;
    }

    return false;
}

bool fast_solve_sudoku(struct sudoku_state* state, int (*grid)[SUDOKU_DIMENSIONS])
{
    return sudoku_state_init(state, grid) &&
           fast_solve_sudoku_impl(state, 0, 0);
}

int solveSudoku(int (*grid)[9], int row, int col)
{
    if (row == 9)
    {
        return 1;
    }
    if (col == 9)
    {
        return solveSudoku(grid, row + 1, 0);
    }
    if (grid[row][col] != 0)
    {
        return solveSudoku(grid, row, col + 1);
    }
    int numbers[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    for (int i = 8; i > 0; i--)
    {
        int random = rand() % (i + 1);
        int temp = numbers[i];
        numbers[i] = numbers[random];
        numbers[random] = temp;
    }
    for (int i = 0; i < 9; i++)
    {
        if (checkValidPlacement(grid, numbers[i], row, col))
        {
            grid[row][col] = numbers[i];
            if (solveSudoku(grid, row, col + 1))
            {
                return 1;
            }
            grid[row][col] = 0;
        }
    }
    return 0;
}

int copyMatrix(int (*grid1)[9], int (*grid2)[9])
{
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            grid1[i][j] = grid2[i][j];
        }
    }
    return 0;
}

int* shuffle()
{
    static int numbers[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    for (int i = 0; i < 9; i++)
    {
        int random = rand() % 9;
        int temp = numbers[i];
        numbers[i] = numbers[random];
        numbers[random] = temp;
    }
    return numbers;
}

int checkValidPlacement(int (*grid)[9], int num, int row, int col)
{
    return checkRows(grid, num, row, col) &&
        checkCols(grid, num, row, col) &&
        checkSubGrid(grid, num, row, col);
}

int generateValidSudoku(int (*grid)[9], int row, int col)
{
    if (row == 9)
    {
        return 1;
    }
    if (col == 9)
    {
       return generateValidSudoku(grid, row + 1, 0);
    }

    int numbers[SUDOKU_DIMENSIONS];
    memcpy(numbers, shuffle(), sizeof numbers);

    for (int i = 0; i < 9; i++)
    {
        int num = numbers[i];

        if (checkValidPlacement(grid, num, row, col))
        {
            grid[row][col] = num;
            if (generateValidSudoku(grid, row, col + 1))
            {
                return 1;
            }
            grid[row][col] = 0;
        }
    }
    return 0;
}

int checkRows(int (*grid)[9], int number, int row, int col)
{
    for (int i = 0; i < 9; i++)
    {
        if (i != col && grid[row][i] == number)
        {
            return false;
        }
    }
    return true;
}

int checkCols(int (*grid)[9], int number, int row, int col)
{
    for (int i = 0; i < 9; i++)
    {
        if (i != row && grid[i][col] == number)
        {
            return false;
        }
    }
    return true;
}

int checkSubGrid(int (*grid)[9], int number, int row, int col)
{
    int first = (row / 3) * 3;
    int second = (col / 3) * 3;

    for (int i = first; i < first + 3; i++)
    {
        for (int j = second; j < second + 3; j++)
        {
            if ((i != row || j != col) && grid[i][j] == number)
            {
                return false;
            }
        }
    }
    return true;
}

