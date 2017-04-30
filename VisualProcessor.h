//
// Created by Max Cowan on 3/21/17.
//

#ifndef SUDOKUSOLVER_VISUALPROCESSOR_H
#define SUDOKUSOLVER_VISUALPROCESSOR_H

#include <iostream>
#include <stdio.h>

#include "SudokuSolver.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include <opencv2/imgproc/imgproc.hpp>

#include <string>
#include <set>
#include <vector>
#include <unordered_set>
#include <string>
#include <map>
#include "tesseract/baseapi.h"
#include <leptonica/allheaders.h>

#include <cmath>

using namespace cv;
using namespace std;


class VisualProcessor {

public:
    // Capture image stream and run CV algorithms on the images
    void analyzeFrames();

    // Find maximum area rectangular contour in hopes of locating the board
    Rect detectPuzzleBounds(Mat detectedEdges);

    // Edit puzzleBounds
    void setPuzzleBounds(Rect bounds);

    // Crop out the grayscale board based on the puzzle rectangle boundaries
    void cropBoard(Mat grayscale_mat);

    // Populates the unwarped_puzzle mat with a thresholded, unwarped image of the puzzle wihch is ready for OCR
    // Used a method to unwarp the puzzle created by Utkarsh Sinha for a tutorial on www.aishack.in
    void unwarpPuzzle(Mat webcam_frame);

    // Split the unwarped board into 81 squares, then run OCR on each square to interpret the givens from the puzzle
    void scanBoard(Mat webcam_frame, tesseract::TessBaseAPI &ocr);

    // Pick color based on number
    Scalar pickColor(int solutionValue);

    // Use tesseract OCR api to return an integer from 0-9 as a detected character
    int recognize_digit(Mat &digit, tesseract::TessBaseAPI &tess);

    // Overlay the transparent warped_solution onto the webcam frame
    void projectSolution(Mat webcam_frame);

    // Transform all black pixels to transparent alpha value
    void alphaSolution(Mat frame);

    // Method to overlay transparent/semi transparent foreground images to a BGR background
    // from http://jepsonsblog.blogspot.com/
    void overlayImage(const cv::Mat &background, const cv::Mat &foreground, cv::Mat &output, cv::Point2i location);

    // Default constructor
    VisualProcessor();


private:
    // Stores the solution image warped to fit the puzzle bounds
    Mat warped_solution;

    // Stores a single digit on the puzzle to run character recognition on
    Mat temp_digit;

    // Contains the puzzle from the input image, and some whitespace around the puzzle if rotation is present
    Mat cropped_puzzle;

    // Contains the square, unwapred puzzle, which compensates for skewed perspecitve in source image
    Mat unwarped_puzzle;

    // Rectangle that contains the puzzle boundaries
    Rect puzzleBounds;

    // Points that hold information for warping
    Point2f src[4], dst[4];

    // Stores the solution of the current puzzle
    vector<vector<int> > solution;

    // Values for various CV transformations, can be adjusted if necessary
    int scale = 3;
    int kernelSize = 3;
    int cannyLowerBound = 30;
};


#endif //SUDOKUSOLVER_VISUALPROCESSOR_H
