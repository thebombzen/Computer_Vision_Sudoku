//
// Created by Max Cowan on 3/4/17.
//

#include "SudokuSolver.h"


using namespace std;

bool SudokuSolver::getSolved(){
    return isSolved;
}

bool SudokuSolver::locateEmptySquare(int &currentRow, int &currentColumn) {
    for (currentRow = 0; currentRow < boardLength; currentRow++) {
        for (currentColumn = 0; currentColumn < boardLength; currentColumn++) {
            if (board[currentRow][currentColumn] == 0) {
                return true;
            }
        }
    }
    //cout << "Solved Puzzle:" << endl; <---- For debugging
    isSolved = true;
    //printBoard(board); <---- For debugging
    return false;
}

bool SudokuSolver::isValidPuzzle(){
    int counter;
    for(int i = 1; i<10; i++){
        for(int row = 0; row < boardLength; row++){
            counter = 0;
            for (int col = 0; col <  boardLength; col++){
                if(board[row][col] == i){
                    counter += 1;
                }
            }
            if (counter > 1){
                return false;
            }
        }
        for(int col = 0; col < boardLength; col ++){
            counter = 0;
            for (int row = 0; row <  boardLength; row++){
                if(board[row][col] == i){
                    counter += 1;
                }
            }
            if (counter > 1){
                return false;
            }
        }
    }
    return true;
}

bool SudokuSolver::isLegalMove(int num, int row, int col) {
    int boxRow = row - (row % 3); //determines the starting row of the 'sub box'
    int boxCol = col - (col % 3); //determines the starting column of the 'sub box'
    if (legalRow(num, row) && legalCol(num, col) && legalSubBox(num, boxRow, boxCol)) {
        return true;
    } else {
        return false;
    }
}

bool SudokuSolver::legalRow(int num, int row) {
    for (int col = 0; col < boardLength; col++) {
        if (board[row][col] == num) {
            return false;
        }
    }
    return true;
}

bool SudokuSolver::legalCol(int num, int col) {
    for (int row = 0; row < boardLength; row++) {
        if (board[row][col] == num) {
            return false;
        }
    }
    return true;
}

bool SudokuSolver::legalSubBox(int num, int firstRow, int firstColumn) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i + firstRow][j + firstColumn] == num) {
                return false;
            }
        }
    }
    return true;
}

// Prints out the current state of the board
void SudokuSolver::printBoard(vector<vector<int> > puzzle) {
    for (int row = 0; row < boardLength; row++) {
        for (int col = 0; col < boardLength; col++) {
            cout << puzzle[row][col] << " ";
        }
        cout << endl;
    }
    cout << endl;
}

vector<vector<int> > SudokuSolver::getBoard(){
    return board;
}

bool SudokuSolver::solvePuzzle() {

    // These hold the coordinates of the square being tested in the function call so that backtracking works properly
    int currentRow;
    int currentCol;

    if (!locateEmptySquare(currentRow, currentCol)) {
        return true;
    }
    for (int guess = 1; guess <= boardLength; guess++) {
        if (isLegalMove(guess, currentRow, currentCol)) {
            board[currentRow][currentCol] = guess;
            if (solvePuzzle()) {
                return true;
            }
            board[currentRow][currentCol] = 0;

        }
    }
    return false;
}

SudokuSolver::SudokuSolver(vector<vector<int> > puzzle) {
    boardLength = 9;
    board = puzzle;
    isSolved = false;

    //cout << "Unsolved Puzzle:" << endl; <---- For debugging
    //printBoard(puzzle);  <---- For debugging

    // Solves puzzle or prints that there isn't a solution
    if (isValidPuzzle()) {
        if (!solvePuzzle()) {
            //cout << "There is no legal solution to this puzzle." << endl; <---- For debugging
        }
    }
    else{
        //cout << "Invalid Puzzle" << endl; <---- For debugging
    }
}

