//
// Created by Max Cowan on 3/21/17.
//

#include "VisualProcessor.h"


using namespace std;
using namespace cv;

void VisualProcessor::analyzeFrames() {

    // Initialize a webcam capture
    VideoCapture capture(0);

    // Instantiate Tesseract OCR
    tesseract::TessBaseAPI myOCR;
    if (myOCR.Init("/usr/local/opt/tesseract/share/tessdata",
                   "eng")) {  //Filepath should be where you store your 'tessdata' file
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }
    myOCR.SetVariable("tessedit_char_whitelist", "1234567890");

    // Initialize frame container
    Mat my_frame, gray, gray_blur, canny_edge;

    while (true) {
        capture >> my_frame;

        if (!my_frame.empty()){
            cvtColor(my_frame, gray, CV_BGR2GRAY); // Convert to grayscale
            blur(gray, gray_blur, Size(3, 3)); // Blur the grayscale
            Canny(gray_blur, canny_edge, cannyLowerBound, cannyLowerBound * scale, kernelSize); // Canny edge detection
            setPuzzleBounds(detectPuzzleBounds(canny_edge)); // Store the rectangular boundaries of the puzzle blob

            // If the rectangle has an area greater than 120,000 pixels and is a square, we assume the puzzle is detected
            if (puzzleBounds.height * puzzleBounds.width > 120000
                && abs(puzzleBounds.height - puzzleBounds.width) / (puzzleBounds.height + puzzleBounds.width) < 0.01) {
                cropBoard(gray); // Crop the puzzle out based on rectangular boundaries
                unwarpPuzzle(my_frame); // Produce the square board image
                scanBoard(my_frame, myOCR); // Scan the puzzle for numbers and produce a solution Mat
                if (!warped_solution.empty()) {
                    projectSolution(my_frame);
                }

            }

            // Display the resultant webcam frame
            imshow("Webcam", my_frame);
            // Delay
            waitKey(3);
        }
        else{
            break;
        }

    }
}

// Crop out the grayscale board based on the puzzle rectangle boundaries
void VisualProcessor::cropBoard(Mat grayscale_mat) {
    Mat tempBlur;
    GaussianBlur(grayscale_mat, tempBlur, Size(11, 11), 0);
    adaptiveThreshold(tempBlur(puzzleBounds), cropped_puzzle, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 7, 2);
    bitwise_not(cropped_puzzle, cropped_puzzle);
}

// Find maximum area rectangular contour in hopes of locating the board
Rect VisualProcessor::detectPuzzleBounds(Mat detectedEdges) {
    double maxArea = 0;
    Rect tempMaxRect;
    vector<vector<Point>> contours;

    findContours(detectedEdges, contours, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++) {
        double rectArea = contourArea(contours[i]);
        if (rectArea > maxArea) {
            maxArea = rectArea;
            tempMaxRect = boundingRect(contours[i]);
        }
    }
    return tempMaxRect;
}

// Populates the unwarped_puzzle mat with a thresholded, unwarped image of the puzzle wihch is ready for OCR
// Used a method to unwarp the puzzle created by Utkarsh Sinha for a tutorial on www.aishack.in
void VisualProcessor::unwarpPuzzle(Mat webcam_frame) {

    // List to store lines as a result of a Hough line transform on the cropped board
    vector<Vec2f> puzzleLines;
    vector<Vec2f> *pLines = &puzzleLines;
    HoughLines(cropped_puzzle, puzzleLines, 1, CV_PI / 180, 200);

    // Fuse lines that are very similar to to simplify representation of the puzzle edges
    vector<Vec2f>::iterator current;

    for (current = pLines->begin(); current != pLines->end(); current++) {
        if ((*current)[0] == 0 && (*current)[1] == -100) continue;

        float p1 = (*current)[0];
        float theta1 = (*current)[1];

        // Look at 2 points on a line depending on vertical or horizontal orientation
        Point pt1current, pt2current;

        if (theta1 > CV_PI * 45 / 180 && theta1 < CV_PI * 135 / 180) {
            pt1current.x = 0;
            pt1current.y = p1 / sin(theta1);
            pt2current.x = cropped_puzzle.size().width;
            pt2current.y = -pt2current.x / tan(theta1) + p1 / sin(theta1);
        } else {
            pt1current.y = 0;
            pt1current.x = p1 / cos(theta1);
            pt2current.y = cropped_puzzle.size().height;
            pt2current.x = -pt2current.y / tan(theta1) + p1 / cos(theta1);
        }
        vector<Vec2f>::iterator pos;

        // Loop through all other lines as a point of comparison
        for (pos = pLines->begin(); pos != pLines->end(); pos++) {
            if (*current == *pos) continue;
            if (fabs((*pos)[0] - (*current)[0]) < 20 && fabs((*pos)[1] - (*current)[1]) < CV_PI * 10 / 180) {
                float p = (*pos)[0];
                float theta = (*pos)[1];

                Point pt1, pt2;
                if ((*pos)[1] > CV_PI * 45 / 180 && (*pos)[1] < CV_PI * 135 / 180) {
                    pt1.x = 0;
                    pt1.y = p / sin(theta);
                    pt2.x = cropped_puzzle.size().width;
                    pt2.y = -pt2.x / tan(theta) + p / sin(theta);
                } else {
                    pt1.y = 0;
                    pt1.x = p / cos(theta);
                    pt2.y = cropped_puzzle.size().height;
                    pt2.x = -pt2.y / tan(theta) + p / cos(theta);
                }
                // Fuse two lines if they are considerably similar in position
                if (((double) (pt1.x - pt1current.x) * (pt1.x - pt1current.x)
                     + (pt1.y - pt1current.y) * (pt1.y - pt1current.y) < 64 * 64)
                    && ((double) (pt2.x - pt2current.x) * (pt2.x - pt2current.x)
                        + (pt2.y - pt2current.y) * (pt2.y - pt2current.y) < 64 * 64)) {
                    (*current)[0] = ((*current)[0] + (*pos)[0]) / 2;
                    (*current)[1] = ((*current)[1] + (*pos)[1]) / 2;
                    (*pos)[0] = 0;
                    (*pos)[1] = -100;
                }
            }
        }
    }


    // Set extreme initial values for the edges about to be calculated
    Vec2f topEdge = Vec2f(1000, 1000);
    double topYIntercept = 100000, topXIntercept = 0;
    Vec2f bottomEdge = Vec2f(-1000, -1000);
    double bottomYIntercept = 0, bottomXIntercept = 0;
    Vec2f leftEdge = Vec2f(1000, 1000);
    double leftXIntercept = 100000, leftYIntercept = 0;
    Vec2f rightEdge = Vec2f(-1000, -1000);
    double rightXIntercept = 0, rightYIntercept = 0;

    for (int i = 0; i < puzzleLines.size(); i++) {
        Vec2f current = puzzleLines[i];
        float p = current[0];
        float theta = current[1];
        if (p == 0 && theta == -100) continue;
        double xIntercept, yIntercept;
        xIntercept = p / cos(theta);
        yIntercept = p / (cos(theta) * sin(theta));

        if (theta > CV_PI * 80 / 180 && theta < CV_PI * 100 / 180) {
            if (p < topEdge[0])
                topEdge = current;
            if (p > bottomEdge[0])
                bottomEdge = current;
        } else if (theta < CV_PI * 10 / 180 || theta > CV_PI * 170 / 180) {
            if (xIntercept > rightXIntercept) {
                rightEdge = current;
                rightXIntercept = xIntercept;
            } else if (xIntercept <= leftXIntercept) {
                leftEdge = current;
                leftXIntercept = xIntercept;
            }
        }

    }


    // Pick two points the lines for calculation of their intersection
    Point left1, left2, right1, right2, bottom1, bottom2, top1, top2;
    int height = cropped_puzzle.size().height;
    int width = cropped_puzzle.size().width;
    if (leftEdge[1] != 0) {
        left1.x = 0;
        left1.y = leftEdge[0] / sin(leftEdge[1]);
        left2.x = width;
        left2.y = -left2.x / tan(leftEdge[1]) + left1.y;
    } else {
        left1.y = 0;
        left1.x = leftEdge[0] / cos(leftEdge[1]);
        left2.y = height;
        left2.x = left1.x - height * tan(leftEdge[1]);
    }
    if (rightEdge[1] != 0) {
        right1.x = 0;
        right1.y = rightEdge[0] / sin(rightEdge[1]);
        right2.x = width;
        right2.y = -right2.x / tan(rightEdge[1]) + right1.y;
    } else {
        right1.y = 0;
        right1.x = rightEdge[0] / cos(rightEdge[1]);
        right2.y = height;
        right2.x = right1.x - height * tan(rightEdge[1]);
    }
    bottom1.x = 0;
    bottom1.y = bottomEdge[0] / sin(bottomEdge[1]);
    bottom2.x = width;
    bottom2.y = -bottom2.x / tan(bottomEdge[1]) + bottom1.y;
    top1.x = 0;
    top1.y = topEdge[0] / sin(topEdge[1]);
    top2.x = width;
    top2.y = -top2.x / tan(topEdge[1]) + top1.y;

    // Calculate intersections with the points
    double leftA = left2.y - left1.y;
    double leftB = left1.x - left2.x;
    double leftC = leftA * left1.x + leftB * left1.y;

    double rightA = right2.y - right1.y;
    double rightB = right1.x - right2.x;
    double rightC = rightA * right1.x + rightB * right1.y;

    double topA = top2.y - top1.y;
    double topB = top1.x - top2.x;
    double topC = topA * top1.x + topB * top1.y;

    double bottomA = bottom2.y - bottom1.y;
    double bottomB = bottom1.x - bottom2.x;
    double bottomC = bottomA * bottom1.x + bottomB * bottom1.y;

    // Intersection of left and top
    double detTopLeft = leftA * topB - leftB * topA;
    CvPoint ptTopLeft = cvPoint((topB * leftC - leftB * topC) / detTopLeft, (leftA * topC - topA * leftC) / detTopLeft);
    // Intersection of top and right
    double detTopRight = rightA * topB - rightB * topA;
    CvPoint ptTopRight = cvPoint((topB * rightC - rightB * topC) / detTopRight,
                                 (rightA * topC - topA * rightC) / detTopRight);
    // Intersection of right and bottom
    double detBottomRight = rightA * bottomB - rightB * bottomA;
    CvPoint ptBottomRight = cvPoint((bottomB * rightC - rightB * bottomC) / detBottomRight,
                                    (rightA * bottomC - bottomA * rightC) / detBottomRight);
    // Intersection of bottom and left
    double detBottomLeft = leftA * bottomB - leftB * bottomA;
    CvPoint ptBottomLeft = cvPoint((bottomB * leftC - leftB * bottomC) / detBottomLeft,
                                   (leftA * bottomC - bottomA * leftC) / detBottomLeft);

    // Determine the maximum width and length for unwarped image by finding max vertical and horizontal edge lengths
    int maxLength = (ptBottomLeft.x - ptBottomRight.x) * (ptBottomLeft.x - ptBottomRight.x) +
                    (ptBottomLeft.y - ptBottomRight.y) * (ptBottomLeft.y - ptBottomRight.y);
    int temp = (ptTopRight.x - ptBottomRight.x) * (ptTopRight.x - ptBottomRight.x) +
               (ptTopRight.y - ptBottomRight.y) * (ptTopRight.y - ptBottomRight.y);
    if (temp > maxLength) maxLength = temp;
    temp = (ptTopRight.x - ptTopLeft.x) * (ptTopRight.x - ptTopLeft.x) +
           (ptTopRight.y - ptTopLeft.y) * (ptTopRight.y - ptTopLeft.y);
    if (temp > maxLength) maxLength = temp;
    temp = (ptBottomLeft.x - ptTopLeft.x) * (ptBottomLeft.x - ptTopLeft.x) +
           (ptBottomLeft.y - ptTopLeft.y) * (ptBottomLeft.y - ptTopLeft.y);
    if (temp > maxLength) maxLength = temp;

    maxLength = sqrt((double) maxLength);

    // Map out the source and destination points to use in the perspective warp function
    src[0] = ptTopLeft;
    dst[0] = Point2f(0, 0);

    src[1] = ptTopRight;
    dst[1] = Point2f(maxLength - 1, 0);

    src[2] = ptBottomRight;
    dst[2] = Point2f(maxLength - 1, maxLength - 1);

    src[3] = ptBottomLeft;
    dst[3] = Point2f(0, maxLength - 1);


    // Draw the boundaries of the detected puzzle on the webcam source frame
    Scalar rgb = Scalar(0, 255, 0);
    line(webcam_frame, Point(puzzleBounds.x + src[0].x, puzzleBounds.y + src[0].y),
         Point(puzzleBounds.x + src[1].x, puzzleBounds.y + src[1].y), rgb);
    line(webcam_frame, Point(puzzleBounds.x + src[1].x, puzzleBounds.y + src[1].y),
         Point(puzzleBounds.x + src[2].x, puzzleBounds.y + src[2].y), rgb);
    line(webcam_frame, Point(puzzleBounds.x + src[2].x, puzzleBounds.y + src[2].y),
         Point(puzzleBounds.x + src[3].x, puzzleBounds.y + src[3].y), rgb);
    line(webcam_frame, Point(puzzleBounds.x + src[3].x, puzzleBounds.y + src[3].y),
         Point(puzzleBounds.x + src[0].x, puzzleBounds.y + src[0].y), rgb);


    // Create the unwarped mat
    unwarped_puzzle = Mat(Size(maxLength, maxLength), CV_8UC1);

    // Use the warpPerspective with the destination and source points just defined.
    cv::warpPerspective(cropped_puzzle, unwarped_puzzle, cv::getPerspectiveTransform(src, dst),
                        Size(maxLength, maxLength));


}

// Split the unwarped board into 81 squares, then run OCR on each square to interpret the givens from the puzzle
void VisualProcessor::scanBoard(Mat webcam_frame, tesseract::TessBaseAPI &ocr) {

    // Mat to draw the solution onto
    Mat temp_nums(unwarped_puzzle.size(), webcam_frame.type(), Scalar(0, 0, 0));

    // Null board
    vector<vector<int>> detectedClues = {
            {0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0}};

    vector<Point> digitCoords;
    Rect digitLocation;

    // Calculate and store the upper left corner coordinates of each digit and the recognized character
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            digitLocation.x = (i * unwarped_puzzle.size().width / 9) + (unwarped_puzzle.size().width / 35);
            digitLocation.y = (j * unwarped_puzzle.size().height / 9) + (unwarped_puzzle.size().height / 35);
            digitLocation.width = (unwarped_puzzle.size().width / 9) - (2 * (unwarped_puzzle.size().width / 35));
            digitLocation.height = (unwarped_puzzle.size().height / 9) - (2 * (unwarped_puzzle.size().height / 35));
            digitCoords.push_back(Point(digitLocation.x, digitLocation.y));

            // Crop a single digit and run OCR on it
            temp_digit = unwarped_puzzle(digitLocation);
            detectedClues[j][i] = recognize_digit(temp_digit, ocr);

        }
    }

    // Solve the extracted sudoku and grab the puzzle solution
    SudokuSolver mySolver = SudokuSolver(detectedClues);
    if (mySolver.getSolved()) {
        solution = mySolver.getBoard();
    }

    // Draw the solution set onto a mat to be re-warped
    if (solution.size() > 0) {
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                if (detectedClues[i][j] == 0) {
                    putText(temp_nums, to_string(solution[i][j]),
                            cvPoint(unwarped_puzzle.size().width / 40 + digitCoords[(j * 9) + (i)].x,
                                    unwarped_puzzle.size().height / 27 + digitCoords[(j * 9) + (i)].y),
                            FONT_HERSHEY_DUPLEX, 1, pickColor(solution[i][j]), 1, CV_AA);
                }

            }
        }

        // Warp the solution mat in reverse to give the illusion of 3d tracking on the webcam output
        warpPerspective(temp_nums, warped_solution, cv::getPerspectiveTransform(dst, src),
                        Size(cropped_puzzle.size().width, cropped_puzzle.size().height));
    }


}


// Pick color based on number
Scalar VisualProcessor::pickColor(int solutionValue) {
    switch (solutionValue) {
        case 1:
            return Scalar(0, 0, 255); // Red
        case 2:
            return Scalar(153, 0, 153); // Purple
        case 3:
            return Scalar(153, 0, 76); // Dark Purple
        case 4:
            return Scalar(255, 255, 0); // Teal
        case 5:
            return Scalar(0, 128, 255); // Orange
        case 6:
            return Scalar(0, 225, 0); // Green
        case 7:
            return Scalar(0, 239, 255); // Yellow
        case 8:
            return Scalar(255, 0, 0); // Blue
        case 9:
            return Scalar(127, 0, 255); // Magenta
        default:
        	// This should never happen. It's to prevent a warning.
        	return Scalar(0, 0, 0); // Black
    }
}


// Use tesseract OCR api to return an integer from 0-9 as a detected character
int VisualProcessor::recognize_digit(Mat &digit, tesseract::TessBaseAPI &tess) {
    tess.SetImage((uchar *) digit.data, digit.size().width, digit.size().height, digit.channels(),
                  (int) digit.step1());
    tess.Recognize(0);
    const char *out = tess.GetUTF8Text();
    if (out)
        if (out[0] == '1')
            return 1;
        else if (out[0] == '2')
            return 2;
        else if (out[0] == '3')
            return 3;
        else if (out[0] == '4')
            return 4;
        else if (out[0] == '5' or out[0] == 'S' or out[0] == 's')
            return 5;
        else if (out[0] == '6')
            return 6;
        else if (out[0] == '7')
            return 7;
        else if (out[0] == '8')
            return 8;
        else if (out[0] == '9')
            return 9;
        else
            return 0;
    else
        return 0;
}


// Overlay the transparent warped_solution onto the webcam frame
void VisualProcessor::projectSolution(Mat webcam_frame) {

    // Need transparent mat for alpha
    Mat4b transparent_solution;
    cvtColor(warped_solution, transparent_solution, CV_BGR2BGRA);

    // Make black pixels transparent
    alphaSolution(transparent_solution);
    // Copy transparent image to foreground
    overlayImage(webcam_frame, transparent_solution, webcam_frame, Point(puzzleBounds.x, puzzleBounds.y));
}

// Transform all black pixels to transparent alpha value
void VisualProcessor::alphaSolution(Mat frame) {
    for (int y = 0; y < frame.rows; ++y) {
        for (int x = 0; x < frame.cols; ++x) {
            cv::Vec4b &pixel = frame.at<cv::Vec4b>(y, x);
            // if pixel is black
            if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) {
                // set alpha to zero:
                pixel[3] = 0;
            }
        }
    }
}

// Edit puzzleBounds
void VisualProcessor::setPuzzleBounds(Rect bounds) {
    puzzleBounds = bounds;
}


// Method to overlay transparent/semi transparent foreground images to a BGR background
// from http://jepsonsblog.blogspot.com/
void VisualProcessor::overlayImage(const cv::Mat &background, const cv::Mat &foreground,
                                   cv::Mat &output, cv::Point2i location) {
    background.copyTo(output);


    // start at the row indicated by location, or at row 0 if location.y is negative.
    for (int y = std::max(location.y, 0); y < background.rows; ++y) {
        int fY = y - location.y; // because of the translation

        // we are done of we have processed all rows of the foreground image.
        if (fY >= foreground.rows)
            break;

        // start at the column indicated by location,

        // or at column 0 if location.x is negative.
        for (int x = std::max(location.x, 0); x < background.cols; ++x) {
            int fX = x - location.x; // because of the translation.

            // we are done with this row if the column is outside of the foreground image.
            if (fX >= foreground.cols)
                break;

            // determine the opacity of the foregrond pixel, using its fourth (alpha) channel.
            double opacity =
                    ((double) foreground.data[fY * foreground.step + fX * foreground.channels() + 3])

                    / 255.;


            // and now combine the background and foreground pixel, using the opacity,

            // but only if opacity > 0.
            for (int c = 0; opacity > 0 && c < output.channels(); ++c) {
                unsigned char foregroundPx =
                        foreground.data[fY * foreground.step + fX * foreground.channels() + c];
                unsigned char backgroundPx =
                        background.data[y * background.step + x * background.channels() + c];
                output.data[y * output.step + output.channels() * x + c] =
                        backgroundPx * (1. - opacity) + foregroundPx * opacity;
            }
        }
    }
}


// Begins visual processing
VisualProcessor::VisualProcessor() {
    analyzeFrames();
}