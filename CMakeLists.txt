#THIS WILL CHANGE DEPENDING ON HOW YOUR DEPENDENCIES ARE INSTALLED
#(However this worked for me after some tinkering)

cmake_minimum_required(VERSION 3.6)
project(SudokuSolver)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_PREFIX_PATH "/usr/local/opt/opencv3/share/OpenCV") #Locate CV

set(OpenCV_INCLUDE_DIRS "/usr/local/opt/opencv3/include") #CV includes
set(OpenCV_LIBS "/usr/local/opt/opencv3/lib") #CV Lib

set(SOURCE_FILES main.cpp SudokuSolver.cpp SudokuSolver.h VisualProcessor.cpp VisualProcessor.h)

find_package(OpenCV REQUIRED) #new

message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

include_directories(${OpenCV_INCLUDE_DIRS}) #new

add_executable(SudokuSolver ${SOURCE_FILES})

target_link_libraries(SudokuSolver ${OpenCV_LIBS}) # Link OpenCV library
target_link_libraries (SudokuSolver lept tesseract) # Link the tesseract OCR library



