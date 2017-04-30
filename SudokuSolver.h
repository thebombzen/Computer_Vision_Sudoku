//
// Created by Max Cowan on 3/4/17.
//

#ifndef SUDOKUSOLVER_SUDOKUSOLVER_H
#define SUDOKUSOLVER_SUDOKUSOLVER_H

#include <iostream>
#include <vector>


class SudokuSolver {

public:
    //Searches the gameboard for null squares on the board and and store the empty square in the functions instance variables
    bool locateEmptySquare(int &currentRow, int &currentColumn);

    // Returns true if all three legal move conditions are met (unique in col, row, and 'sub box')
    bool isLegalMove(int num, int row, int col);

    // Returns true if the number to be inserted does not exist in the given row
    bool legalRow(int num, int row);

    // Returns true if the number to be inserted does not exist in the given column
    bool legalCol(int num, int col);

    // Returns true fi the number to be inserted does not exist in the indicated 'sub box'
    bool legalSubBox(int num, int firstRow, int firstColumn);

    // Prints out the current state of the board
    void printBoard(std::vector<std::vector<int> > puzzle);

    // Main recursive function which attempts to solve a given puzzle using backtracking techniques
    bool solvePuzzle();

    // returns isSolved
    bool getSolved();

    // is this puzzle valid?
    bool isValidPuzzle();

    // returns the board
    std::vector<std::vector<int> > getBoard();

    // Default constructor assigns boardLength to 9 (like a typical Sudoku puzzle)
    SudokuSolver(std::vector<std::vector<int> > puzzle);

private:

    // isSolved
    bool isSolved;

    // Dimension of the square board and the numbers it contains (EG 9 means 9x9 board and 1-9 to be placed in squares)
    int boardLength;

    // 2D vector to load board into
    std::vector<std::vector<int> > board;
};


#endif //SUDOKUSOLVER_SUDOKUSOLVER_H
