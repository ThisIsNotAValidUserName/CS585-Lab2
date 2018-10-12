#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <io.h> 
#include <iostream>
#include <vector> 
#include <stdio.h>
#include <stdlib.h>
#include <iomanip>

using namespace cv;
using namespace std;

void getFiles(string path, vector<string>& ownname);
bool compareColor(Vec3b a, Vec3b b);
Mat myFindContours(Mat image);
Vec3b findBackgroundColor(Mat image);
Mat fillHoles(Mat src, Mat contour);
void FindBinaryLargeObjects(const Mat &binaryImg, vector <vector<Point2i>> &blobs);
vector<Vec3b> findAllColors(Mat colored);
void determineShape(Mat &recoloredAndSmoothed, Mat &colored, string filename);
void classifyBoders(Mat &coloredWithBorders, Mat &classifiedBorders);
void smoothTheContours(Mat &coloredWithBorders);
void repairTheContours(Mat &recolredAndSmoothed);
void recolorTheShapes(Mat &coloredWithBorders);
void recolorAndSmooth(Mat &coloredWithBorders);
bool isSquare(vector<Point> points);
bool isTriangle(vector<Point> points);
Point calculateCentroid(Mat binary);
void calculatePrecision();

int square = 0;
int triangle = 0;
int rounds = 0;
vector<Vec3b> colors;

string squarepath[] = { "_square_0","_square_1","_square_2","_square_3","_square_c","_square_c_crowd" };
string tripath[] = { "_triangle_0","_triangle_1","_triangle_3","_triangle_2","_triangle_c","_triangle_c_crowd" };
string cirpath[] = { "_circle_0","_circle_1","_circle_3","_circle_2","_circle_c","_circle_c_crowd" };

int main()
{
	vector<string> files;
	getFiles("shapes/", files);
	int size = files.size();
	for (int i = 0; i < size; i++)
	{
		string f = files[i].c_str();
		string filename = "shapes/" + f;
		cout << filename << endl;

		Mat src;
		src = imread(filename, IMREAD_COLOR); // Read the file
		if (src.empty())                      // Check for invalid input
		{
			cout << "Could not open or find the image" << std::endl;
			return -1;
		}

		//Denoise
		blur(src, src, Size(3, 3));

		Mat contour = myFindContours(src);
		imwrite("contours/" + f.substr(0, 4) + ".bmp", contour);

		//Find Background color
		Vec3b bgc = findBackgroundColor(src);
		Mat back(128, 128, CV_8UC3, bgc);
		imwrite("bgc/" + f.substr(0, 4) + ".bmp", back);

		Mat filled = fillHoles(src, contour);
		imwrite("filled/" + f.substr(0, 4) + ".bmp", filled);
		//Read again as thresholded image
		Mat img = imread("filled/" + f.substr(0, 4) + ".bmp", 0);

		/////////////////////////////////////////////////////////////////////////////////
		//convert thresholded image to binary image
		Mat binary;
		threshold(img, binary, 0.0, 1.0, THRESH_BINARY);

		Mat lines = imread("contours/" + f.substr(0, 4) + ".bmp", 0);
		//initialize vector to store all blobs
		vector <vector<Point2i>> blobs;

		//call function that detects blobs and modifies the input binary image to store blob info
		FindBinaryLargeObjects(binary, blobs);

		//display the output
		Mat colored = Mat::zeros(filled.size(), CV_8UC3);
		// Randomly color the blobs

		for (size_t i = 0; i < blobs.size(); i++) {
			unsigned char r = 255 * ((rand() + 1) / (1.0 + RAND_MAX));
			unsigned char g = 255 * ((rand() + 1) / (1.0 + RAND_MAX));
			unsigned char b = 255 * ((rand() + 1) / (1.0 + RAND_MAX));

			for (size_t j = 0; j < blobs[i].size(); j++) {
				int x = blobs[i][j].x;
				int y = blobs[i][j].y;

				colored.at<Vec3b>(y, x)[0] = b;
				colored.at<Vec3b>(y, x)[1] = g;
				colored.at<Vec3b>(y, x)[2] = r;
			}
		}
		/////////////////////////////////////////////////////////////////////////////////
		colors = findAllColors(colored);
		Mat dilateElement = getStructuringElement(MORPH_RECT, Size(4, 4));
		dilate(colored, colored, dilateElement);
		imwrite("colored/" + f.substr(0, 4) + ".bmp", colored);

		//All the colors in the colored image
		vector<Vec3b> colors = findAllColors(colored);

		Mat erodeStruct = getStructuringElement(MORPH_RECT, Size(2, 2));
		erode(contour, contour, erodeStruct);

		Mat coloredWithBorders = colored | contour;
		repairTheContours(coloredWithBorders);
		imwrite("coloredWithBorders/" + f.substr(0, 4) + ".bmp", coloredWithBorders);
		recolorAndSmooth(coloredWithBorders);

		Mat recoloredAndSmoothed = coloredWithBorders;

		imwrite("recoloredAndSmoothed/" + f.substr(0, 4) + ".bmp", recoloredAndSmoothed);

		Mat classifiedBorders(coloredWithBorders.rows, coloredWithBorders.cols, CV_8UC3, Scalar(0, 0, 0));
		classifyBoders(recoloredAndSmoothed, classifiedBorders);
		imwrite("classifiedborders/" + f.substr(0, 4) + ".bmp", classifiedBorders);

		determineShape(recoloredAndSmoothed, colored, f.substr(0, 4));
	}

	
	cout << "Square: " << square << endl;
	cout << "Triangle: " << triangle << endl;
	cout << "Round: " << rounds << endl;
	calculatePrecision();

	getchar();
	return 0;
}

void getFiles(string path, vector<string>& ownname)
{
	//文件句柄  
	intptr_t hFile = 0;
	//文件信息  
	struct _finddata_t fileinfo;
	string p;
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			//如果是目录,迭代之  
			//如果不是,加入列表  
			if ((fileinfo.attrib &  _A_SUBDIR))
			{
				/*
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					getFiles(p.assign(path).append("\\").append(fileinfo.name), files);
					*/
			}
			else
			{
				//files.push_back(p.assign(path).append("\\").append(fileinfo.name));
				ownname.push_back(fileinfo.name);
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
}

bool compareColor(Vec3b a, Vec3b b) {
	int threshod = 10;
	if (abs(a(0) - b(0)) >= threshod || abs(a(1) - b(1)) >= threshod || abs(a(2) - b(2)) >= threshod)
		return true;
	return false;
}

Mat myFindContours(Mat image)
{
	Mat contour_output = Mat::zeros(image.size(), CV_8UC3);
	for (int row = 0; row < image.rows; row++)
	{
		for (int col = 0; col < image.cols; col++)
		{
			if (row == 0 || col == 0 || row == image.rows - 1 || col == image.cols - 1)
			{
				if (image.at<Vec3b>(row, col) != Vec3b(0, 0, 0))
					contour_output.at<Vec3b>(row, col) = Vec3b(255, 255, 255);
				continue;
			}
			//N8
			Vec3b pixels[9];
			pixels[0] = image.at<Vec3b>(row, col);
			pixels[1] = image.at<Vec3b>(row, col - 1);      //west
			pixels[2] = image.at<Vec3b>(row + 1, col - 1);  //southwest
			pixels[3] = image.at<Vec3b>(row + 1, col);      //south
			pixels[4] = image.at<Vec3b>(row + 1, col + 1);  //southeast
			pixels[5] = image.at<Vec3b>(row, col + 1);      //east
			pixels[6] = image.at<Vec3b>(row - 1, col + 1);  //northeast
			pixels[7] = image.at<Vec3b>(row - 1, col);      //north
			pixels[8] = image.at<Vec3b>(row - 1, col - 1);  //northwest

			for (int i = 1; i < 9; i++) {
				if (compareColor(pixels[i], pixels[0]))
				{
					contour_output.at<Vec3b>(row, col) = Vec3b(255, 255, 255);
					break;
				}
			}
		}
	}
	Mat erodeStruct = getStructuringElement(MORPH_RECT, Size(3, 3));
	erode(contour_output, contour_output, erodeStruct);
	return contour_output;
}

Vec3b findBackgroundColor(Mat image)
{
	int rows = image.rows;
	int cols = image.cols;
	vector<Vec3b> color;
	vector<int> num;
	
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if ((i > 0 && i < rows - 1) && (j > 0 && j < cols - 1))
				continue;
			else {
				bool notInVector = true;
				for (int x = 0; x < color.size(); x++) {
					//cout << color.at(x) << endl;
					if (color.at(x) == image.at<Vec3b>(i, j)) {
						num.at(x)++;
						notInVector = false;
						break;
					}
					notInVector = true;
				}
				if (notInVector == true) {
					color.push_back(image.at<Vec3b>(i, j));
					num.push_back(1);
				}
			}
		}
	}

	Vec3b result;
	int number = 0;;
	for (int y = 0; y < color.size(); y++)
	{
		if (num.at(y) > number)
		{
			result = color.at(y);
			number = num.at(y);
		}
	}

	return result;
}

Mat fillHoles(Mat src, Mat contour) {
	Mat filled = contour.clone();
	Vec3b bgc = findBackgroundColor(src);
	int rows = filled.rows;
	int cols = filled.cols;
	for (int row = 0; row < rows; row++) {
		for (int col = 0; col < cols; col++) {
			if (filled.at<Vec3b>(row, col) == Vec3b(255, 255, 255)) {
				filled.at<Vec3b>(row, col) = Vec3b(0, 0, 0);
				continue;
			}
			if (compareColor(src.at<Vec3b>(row, col), bgc))
				filled.at<Vec3b>(row, col) = Vec3b(255, 255, 255);
		}
	}
	//dilate(filled, filled, Mat());
	Mat erodeStruct = getStructuringElement(MORPH_RECT, Size(2, 2));
	erode(filled, filled, erodeStruct);
	//Mat dilateElement = getStructuringElement(MORPH_RECT, Size(3, 3));
	//dilate(filled, filled, dilateElement);
	return filled;
}

void FindBinaryLargeObjects(const Mat &binary, vector <vector<Point2i>> &blobs)
{
	//clear blobs vector
	blobs.clear();

	//labelled image of type CV_32SC1
	Mat label_image = binary.clone();
	binary.convertTo(label_image, CV_32SC1);

	//label count starts at 2
	int label_count = 2;

	//iterate over all pixels until a pixel with a 1-value is encountered
	for (int y = 0; y < label_image.rows; y++) {
		int *row = (int*)label_image.ptr(y);
		for (int x = 0; x < label_image.cols; x++) {
			if (row[x] != 1) {
				continue;
			}
			//cout << x << endl;
			//floodFill the connected component with the label count
			//floodFill documentation: http://docs.opencv.org/modules/imgproc/doc/miscellaneous_transformations.html#floodfill
			Rect rect;
			floodFill(label_image, Point(x, y), label_count, &rect, 0, 0, 4);

			//store all 2D co-ordinates in a vector of 2d points called blob
			vector <Point2i> blob;
			for (int i = rect.y; i < (rect.y + rect.height); i++) {
				int *row2 = (int*)label_image.ptr(i);
				for (int j = rect.x; j < (rect.x + rect.width); j++) {
					if (row2[j] != label_count) {
						continue;
					}
					blob.push_back(Point2i(j, i));
				}
			}
			//store the blob in the vector of blobs
			blobs.push_back(blob);

			//increment counter
			label_count++;
			//cout << blobs.size() << endl;
		}
	}
	//cout << "The number of blobs in the image is: " << label_count;
	//Code derived from: http://nghiaho.com/
}

vector<Vec3b> findAllColors(Mat colored)
{
	vector<Vec3b> colors;
	for (int i = 0; i < colored.rows; i++)
	{
		for (int j = 0; j < colored.cols; j++)
		{
			if (colored.at<Vec3b>(i, j) != Vec3b(0, 0, 0))
			{
				bool notInVector = true;
				for (int x = 0; x < colors.size(); x++)
				{
					if (colors.at(x) == colored.at<Vec3b>(i, j))
					{
						notInVector = false;
						break;
					}
					notInVector = true;
				}
				if (notInVector == true)
					colors.push_back(colored.at<Vec3b>(i, j));
			}
		}
	}
	return colors;
}

void determineShape(Mat &recoloredAndSmoothed, Mat &colored, string filename)
{
	int Total = -1;
	int rows = recoloredAndSmoothed.rows;
	int cols = recoloredAndSmoothed.cols;
	for (int i = 0; i < colors.size(); i++)
	{
		Point startingPoint;
		Vec3b color = colors.at(i);
		vector<Point> points;
		for (int row = 1; row < rows - 1; row++)
		{
			for (int col = 1; col < cols - 1; col++)
			{
				if ((recoloredAndSmoothed.at<Vec3b>(row, col) == Vec3b(255, 255, 255) && (recoloredAndSmoothed.at<Vec3b>(row + 1, col) == color
					|| recoloredAndSmoothed.at<Vec3b>(row, col + 1) == color
					|| recoloredAndSmoothed.at<Vec3b>(row - 1, col) == color
					|| recoloredAndSmoothed.at<Vec3b>(row, col - 1) == color) && (
						recoloredAndSmoothed.at<Vec3b>(row, col + 1) == Vec3b(0, 0, 0)
						|| recoloredAndSmoothed.at<Vec3b>(row, col - 1) == Vec3b(0, 0, 0))))
				{
					points.push_back(Point(col, row));
					if (points.size() == 32)
						goto loop;
					break;
				}
			}
		}
		if (points.size() <= 10)
			continue;
	loop:
		cout << endl << color << endl;

		Mat output(rows, cols, CV_8UC3, Scalar(0, 0, 0));
		string shape;

		if (isSquare(points)) {
			cout << "isSquare" << endl;
			square++;
			Total++;
			shape = "square";
		}
		else if (isTriangle(points)) {
			cout << "isTriangle" << endl;
			triangle++;
			Total++;
			shape = "triangle";
		}
		else {
			cout << "isCircle" << endl;
			rounds++;
			Total++;
			shape = "circle";
		}

		for (int i = 0; i < rows; i++)
		{
			for (int j = 0; j < cols; j++)
			{
				if (colored.at<Vec3b>(i, j) == color) {
					//cout << color << endl;
					output.at<Vec3b>(i, j) = Vec3b(255, 255, 255);
				}
			}
		}
		imwrite("outcome/" + filename + "_" + shape + "_" + std::to_string(Total) + ".bmp", output);
	}
}

void classifyBoders(Mat &coloredWithBorders, Mat &classifiedBorders)
{
	//White —— blob and background
	//Blue —— blob and blob
	//Red —— blob and boundary of the image
	int row = coloredWithBorders.rows;
	int col = coloredWithBorders.cols;
	for (int x = 0; x < row; x++)
	{
		for (int y = 0; y < col; y++)
		{
			int blackCount = 0;
			if (x == 0 || y == 0 || x == row - 1 || y == col - 1)
			{
				if (coloredWithBorders.at<Vec3b>(x, y) != Vec3b(0, 0, 0))
					classifiedBorders.at<Vec3b>(x, y) = Vec3b(0, 0, 255);
			}
			else
			{
				Vec3b pixels[9];
				Vec3b a = Vec3b(0, 0, 0);
				Vec3b b = Vec3b(0, 0, 0);
				pixels[0] = coloredWithBorders.at<Vec3b>(x, y);
				pixels[1] = coloredWithBorders.at<Vec3b>(x, y - 1);      //west
				pixels[2] = coloredWithBorders.at<Vec3b>(x + 1, y - 1);  //southwest
				pixels[3] = coloredWithBorders.at<Vec3b>(x + 1, y);      //south
				pixels[4] = coloredWithBorders.at<Vec3b>(x + 1, y + 1);  //southeast
				pixels[5] = coloredWithBorders.at<Vec3b>(x, y + 1);      //east
				pixels[6] = coloredWithBorders.at<Vec3b>(x - 1, y + 1);  //northeast
				pixels[7] = coloredWithBorders.at<Vec3b>(x - 1, y);      //north
				pixels[8] = coloredWithBorders.at<Vec3b>(x - 1, y - 1);  //northwest

				if (coloredWithBorders.at<Vec3b>(x, y) == Vec3b(255, 255, 255))
				{
					for (int i = 1; i < 9; i++) {
						if (pixels[i] == Vec3b(0, 0, 0))
							blackCount++;
						else if (pixels[i] != Vec3b(255, 255, 255) && pixels[i] != Vec3b(255, 0, 0) && pixels[i] != Vec3b(0, 0, 255)) {
							if (a == Vec3b(0, 0, 0))
								a = pixels[i];
							else
								b = pixels[i];
						}
					}
					if (blackCount == 0 && b != Vec3b(0, 0, 0))
						classifiedBorders.at<Vec3b>(x, y) = Vec3b(255, 255, 255);
					else
						classifiedBorders.at<Vec3b>(x, y) = Vec3b(255, 0, 0);
				}
			}
		}
	}
}

void smoothTheContours(Mat &coloredWithBorders)
{
	//Narrow the width of borders to 1 pixel
	int row = coloredWithBorders.rows;
	int col = coloredWithBorders.cols;
	for (int x = 1; x < row - 1; x++)
	{
		for (int y = 1; y < col - 1; y++)
		{
			//cout << x << "   " << y << endl;
			int whiteCount = 0;
			int blackCount = 0;
			Vec3b pixels[9];
			pixels[0] = coloredWithBorders.at<Vec3b>(x, y);
			pixels[1] = coloredWithBorders.at<Vec3b>(x, y - 1);      //west
			pixels[2] = coloredWithBorders.at<Vec3b>(x + 1, y - 1);  //southwest
			pixels[3] = coloredWithBorders.at<Vec3b>(x + 1, y);      //south
			pixels[4] = coloredWithBorders.at<Vec3b>(x + 1, y + 1);  //southeast
			pixels[5] = coloredWithBorders.at<Vec3b>(x, y + 1);      //east
			pixels[6] = coloredWithBorders.at<Vec3b>(x - 1, y + 1);  //northeast
			pixels[7] = coloredWithBorders.at<Vec3b>(x - 1, y);      //north
			pixels[8] = coloredWithBorders.at<Vec3b>(x - 1, y - 1);  //northwest

			if (pixels[0] == Vec3b(255, 255, 255))
			{
				for (int i = 1; i < 9; i++) {
					if (pixels[i] == Vec3b(255, 255, 255))
						whiteCount++;
					else if (pixels[i] == Vec3b(0, 0, 0))
						blackCount++;
				}
			}
			if (whiteCount > 5 || blackCount > 5)
				coloredWithBorders.at<Vec3b>(x, y) = Vec3b(0, 0, 0);
			if (whiteCount + blackCount == 8)
				coloredWithBorders.at<Vec3b>(x, y) = Vec3b(0, 0, 0);
		}
	}
}

void repairTheContours(Mat &recolredAndSmoothed) {
	int row = recolredAndSmoothed.rows;
	int col = recolredAndSmoothed.cols;
	for (int x = 1; x < row - 1; x++) {
		for (int y = 1; y < col - 1; y++) {
			//cout << x << "   " << y << endl;
			Vec3b color;
			Vec3b pixels[9];
			pixels[0] = recolredAndSmoothed.at<Vec3b>(x, y);
			pixels[1] = recolredAndSmoothed.at<Vec3b>(x, y - 1);      //west
			pixels[2] = recolredAndSmoothed.at<Vec3b>(x + 1, y - 1);  //southwest
			pixels[3] = recolredAndSmoothed.at<Vec3b>(x + 1, y);      //south
			pixels[4] = recolredAndSmoothed.at<Vec3b>(x + 1, y + 1);  //southeast
			pixels[5] = recolredAndSmoothed.at<Vec3b>(x, y + 1);      //east
			pixels[6] = recolredAndSmoothed.at<Vec3b>(x - 1, y + 1);  //northeast
			pixels[7] = recolredAndSmoothed.at<Vec3b>(x - 1, y);      //north
			pixels[8] = recolredAndSmoothed.at<Vec3b>(x - 1, y - 1);  //northwest

			if (pixels[0] == Vec3b(0, 0, 0)) {
				for (int i = 1; i < 9; i++) {
					if (pixels[i] != Vec3b(0, 0, 0) && pixels[i] != Vec3b(255, 255, 255)) {
						recolredAndSmoothed.at<Vec3b>(x, y) = Vec3b(255, 255, 255);
					}
				}
			}
		}
	}
}

void recolorTheShapes(Mat &coloredWithBorders)
{
	int row = coloredWithBorders.rows;
	int col = coloredWithBorders.cols;
	for (int x = 1; x < row - 1; x++) {
		for (int y = 1; y < col - 1; y++) {
			//cout << x << "   " << y << endl;
			int whiteCount = 0;
			int blackCount = 0;
			Vec3b color;
			Vec3b pixels[9];
			pixels[0] = coloredWithBorders.at<Vec3b>(x, y);
			pixels[1] = coloredWithBorders.at<Vec3b>(x, y - 1);      //west
			pixels[2] = coloredWithBorders.at<Vec3b>(x + 1, y - 1);  //southwest
			pixels[3] = coloredWithBorders.at<Vec3b>(x + 1, y);      //south
			pixels[4] = coloredWithBorders.at<Vec3b>(x + 1, y + 1);  //southeast
			pixels[5] = coloredWithBorders.at<Vec3b>(x, y + 1);      //east
			pixels[6] = coloredWithBorders.at<Vec3b>(x - 1, y + 1);  //northeast
			pixels[7] = coloredWithBorders.at<Vec3b>(x - 1, y);      //north
			pixels[8] = coloredWithBorders.at<Vec3b>(x - 1, y - 1);  //northwest

			if (pixels[0] != Vec3b(255, 255, 255))
				continue;
			else {
				//Upper Three
				if (pixels[8] == Vec3b(255, 255, 255) && pixels[7] == Vec3b(255, 255, 255) && pixels[6] == Vec3b(255, 255, 255)) {
					//Northwest
					if (pixels[2] == Vec3b(255, 255, 255) && pixels[1] == Vec3b(255, 255, 255) && pixels[5] == pixels[4] && pixels[4] == pixels[3]) {
						coloredWithBorders.at<Vec3b>(x, y) = pixels[3];
					}
					//North
					else if (pixels[1] == pixels[2] && pixels[2] == pixels[3] && pixels[3] == pixels[4] && pixels[4] == pixels[5]) {
						coloredWithBorders.at<Vec3b>(x, y) = pixels[3];
					}
					//Northeast
					if (pixels[4] == Vec3b(255, 255, 255) && pixels[5] == Vec3b(255, 255, 255) && pixels[1] == pixels[2] && pixels[2] == pixels[3]) {
						coloredWithBorders.at<Vec3b>(x, y) = pixels[3];
					}
				}
				//Middle Left
				else if (pixels[8] == Vec3b(255, 255, 255) && pixels[1] == Vec3b(255, 255, 255) && pixels[2] == Vec3b(255, 255, 255)
					&& pixels[3] == pixels[4] && pixels[4] == pixels[5] && pixels[5] == pixels[6] && pixels[6] == pixels[7]) {
					coloredWithBorders.at<Vec3b>(x, y) = pixels[3];
				}
				//Middle Right
				else if (pixels[4] == Vec3b(255, 255, 255) && pixels[5] == Vec3b(255, 255, 255) && pixels[6] == Vec3b(255, 255, 255)
					&& pixels[1] == pixels[2] && pixels[2] == pixels[3] && pixels[3] == pixels[7] && pixels[7] == pixels[8]) {
					coloredWithBorders.at<Vec3b>(x, y) = pixels[3];
				}
				//Nether Three
				else if (pixels[2] == Vec3b(255, 255, 255) && pixels[3] == Vec3b(255, 255, 255) && pixels[4] == Vec3b(255, 255, 255)) {
					//Southwest
					if (pixels[8] == Vec3b(255, 255, 255) && pixels[1] == Vec3b(255, 255, 255) && pixels[5] == pixels[6] && pixels[6] == pixels[7]) {
						coloredWithBorders.at<Vec3b>(x, y) = pixels[7];
					}
					//south
					else if (pixels[1] == pixels[5] && pixels[5] == pixels[6] && pixels[6] == pixels[7] && pixels[7] == pixels[8]) {
						coloredWithBorders.at<Vec3b>(x, y) = pixels[7];
					}
					//Southeast
					if (pixels[5] == Vec3b(255, 255, 255) && pixels[6] == Vec3b(255, 255, 255) && pixels[1] == pixels[7] && pixels[7] == pixels[8]) {
						coloredWithBorders.at<Vec3b>(x, y) = pixels[7];
					}
				}

				for (int i = 1; i < 9; i++) {
					if (pixels[i] == Vec3b(255, 255, 255))
						whiteCount++;
					else if (pixels[i] != Vec3b(0, 0, 0))
						color = pixels[i];
					else if (pixels[i] == Vec3b(0, 0, 0))
						blackCount++;
				}
				//if (blackCount + whiteCount == 8)
				//	coloredWithBorders.at<Vec3b>(x, y) = Vec3b(0, 0, 0);
				if ((whiteCount > 5 || whiteCount < 2) && blackCount == 0)
					coloredWithBorders.at<Vec3b>(x, y) = color;
			}
		}
	}
}

void recolorAndSmooth(Mat &coloredWithBorders)
{
	recolorTheShapes(coloredWithBorders);
	recolorTheShapes(coloredWithBorders);
	smoothTheContours(coloredWithBorders);
	smoothTheContours(coloredWithBorders);
}

bool isSquare(vector<Point> points) {
	vector<double> slopes;
	int count = 0;
	int consecutiveZero = 0;
	int consecutiveZeroMax = 0;
	Point startingPoint = points.at(0);

	for (int j = 1; j < points.size(); j++) {
		if (startingPoint.y - points.at(j).y != 0) {
			slopes.push_back((double)(startingPoint.x - points.at(j).x) / (startingPoint.y - points.at(j).y));
			cout << "Starting point" << startingPoint << endl;
			cout << points.at(j) << endl;
			cout << "SlopeSquare: " << (double)(startingPoint.x - points.at(j).x) / (startingPoint.y - points.at(j).y) << endl;
			startingPoint = points.at(j);
		}
		else
			slopes.push_back(0);
	}

	for (int i = 0; i < slopes.size(); i++) {
		if (slopes.at(i) == 0) {
			count++;
			consecutiveZero++;
		}
		else {
			if (consecutiveZero > consecutiveZeroMax)
				consecutiveZeroMax = consecutiveZero;
			consecutiveZero = 0;
		}
	}

	double divide = (double)count / (double)slopes.size();
	cout << "Divide: " << divide << endl;

	if (count != 0 && divide >= 0.74)
		return true;
	else
		return false;
}

bool isTriangle(vector<Point> points)
{
	vector<double> slopes;
	//Left Border
	int countNegative = 0;
	//Right Border
	int countPositive = 0;
	Point startingPoint = points.at(0);

	for (int j = 1; j < points.size(); j++)
	{
		if (startingPoint.y - points.at(j).y != 0) {
			slopes.push_back((double)(startingPoint.x - points.at(j).x) / (startingPoint.y - points.at(j).y));
			cout << "Starting point" << startingPoint << endl;
			cout << points.at(j) << endl;
			cout << "SlopeTriangle: " << (double)(startingPoint.x - points.at(j).x) / (startingPoint.y - points.at(j).y) << endl;
		}
		else
			slopes.push_back(0);
	}

	for (int i = 0; i < slopes.size(); i++) {
		if (slopes.at(i) >= -0.6 && slopes.at(i) <= -0.5)
			countNegative++;
		else if (slopes.at(i) >= 0.5 && slopes.at(i) <= 0.6)
			countPositive++;
	}

	double divideNegative = (double)countNegative / (double)slopes.size();
	double dividePositive = (double)countPositive / (double)slopes.size();
	double divide = (double)(countNegative + countPositive) / (double)slopes.size();

	cout << "DivideNegative: " << divideNegative << endl;
	cout << "DividePositive: " << dividePositive << endl;
	cout << "Divide: " << divide << endl;

	if (divideNegative >= 0.25 || dividePositive >= 0.25 || divide >= 0.25)
		return true;
	else
		return false;
}

Point calculateCentroid(Mat binary) {

	// find moments of the image
	Moments m = moments(binary, true);
	// coordinates of centroid
	Point p(m.m10 / m.m00, m.m01 / m.m00);

	return p;
}

void calculatePrecision() {
	//Calculate Precision
	vector<string> files;
	getFiles("outcome/", files);
	int size2 = files.size();
	int circlefp = 0;
	int circletp = 0;
	int squarefp = 0;
	int squaretp = 0;
	int trianglefp = 0;
	int triangletp = 0;
	for (int i = 0; i < files.size(); i++)
	{
		string f = files[i].c_str();
		string filename = "outcome/" + f;
		cout << filename << endl;
		string num = f.substr(0, 4);
		string shape = f.substr(5, 6);
		cout << "shape: " << shape << endl;

		Mat src;
		src = imread(filename, 0); // Read the file
		if (src.empty())                      // Check for invalid input
			cout << "Could not open or find the image" << std::endl;

		bool flag = false;
		Point centroid = calculateCentroid(src);
		cout << centroid << endl;

		if (shape == "circle") {
			for (int i = 0; i < 6; i++) {
				Mat anno = imread("annotations/" + num + cirpath[i] + ".png", 0); // Read the file
				if (!anno.empty()) {
					uchar color = anno.at<uchar>(centroid.y, centroid.x);
					if (color == 255 && !flag) {
						circletp++;
						flag = true;
					}
				}
			}
			if (!flag)
				circlefp++;
		}
		else if (shape == "triang") 
		{
			for (int i = 0; i < 6; i++) {
				Mat anno = imread("annotations/" + num + tripath[i] + ".png", 0); // Read the file
				if (!anno.empty()) {
					uchar color = anno.at<uchar>(centroid.y, centroid.x);
					if (color == 255 && !flag) {
						triangletp++;
						flag = true;
					}
				}
			}
			if (!flag)
				trianglefp++;
		}
		else if (shape == "square")
		{
			for (int i = 0; i < 6; i++) {
				Mat anno = imread("annotations/" + num + squarepath[i] + ".png", 0); // Read the file
				if (!anno.empty()) {
					uchar color = anno.at<uchar>(centroid.y, centroid.x);
					if (color == 255 && !flag) {
						squaretp++;
						flag = true;
					}
				}
			}
			if (!flag)
				squarefp++;
		}
	}

	double precision = (double)(squaretp + circletp + triangletp) / 1074;

	cout << "Squaretp: " << squaretp << endl;
	cout << "Squarefp: " << squarefp << endl;
	cout << "Triangletp: " << triangletp << endl;
	cout << "Trianglefp: " << trianglefp << endl;
	cout << "Circletp: " << circletp << endl;
	cout << "Circlefp: " << circlefp << endl;
	cout << "Precision: " << precision;
}