// OpenCVApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "common.h"
#include <unordered_map>


void testOpenImage()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src;
		src = imread(fname);
		imshow("opened image", src);
		waitKey();
	}
}

void testOpenImagesFld()
{
	char folderName[MAX_PATH];
	if (openFolderDlg(folderName) == 0)
		return;
	char fname[MAX_PATH];
	FileGetter fg(folderName, "bmp");
	while (fg.getNextAbsFile(fname))
	{
		Mat src;
		src = imread(fname);
		imshow(fg.getFoundFileName(), src);
		if (waitKey() == 27) //ESC pressed
			break;
	}
}

void testColor2Gray()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat_<Vec3b> src = imread(fname, IMREAD_COLOR);

		int height = src.rows;
		int width = src.cols;

		Mat_<uchar> dst(height, width);

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				Vec3b v3 = src(i, j);
				uchar b = v3[0];
				uchar g = v3[1];
				uchar r = v3[2];
				dst(i, j) = (r + g + b) / 3;
			}
		}

		imshow("original image", src);
		imshow("gray image", dst);
		waitKey();
	}
}


/////////////////////////////////////////////////////////////////////////////////
//******************************************* PROJECT ***********************////   
/////////////////////////////////////////////////////////////////////////////////




int isInside(cv::Point p, int rows, int cols) {
	if (p.x < rows && p.y < cols)
		return 1;
	return 0;
}

// deseneaaza liniile dintr un vector de la poitia de inceput la final
Mat drawLinesOnImageV2(int rows, int cols, std::vector<Vec4i> lines) {
	Mat_<uchar> dst(rows, cols, 255);
	for (size_t i = 0; i < lines.size(); i++)
	{
		line(dst, Point(lines[i][0], lines[i][1]),
			Point(lines[i][2], lines[i][3]), 0, 1);
	}
	return dst;
}


Mat drawLinesOnImageV1(int rows, int cols, std::vector<Vec2f> detected_lines)
{
	Mat_<uchar> dst(rows, cols, 255);

	for (auto line1 : detected_lines)
	{
		float rho = line1[0];
		float theta = line1[1];

		double a = cos(theta), b = sin(theta);
		double x0 = a * rho, y0 = b * rho;
		Point pt1(cvRound(x0 + 1000 * (-b)), cvRound(y0 + 1000 * (a)));
		Point pt2(cvRound(x0 - 1000 * (-b)), cvRound(y0 - 1000 * (a)));

		line(dst, pt1, pt2, 0, 1);
	}
	return dst;
}

int isParallel(float m1, float m2, float threshold)
{
	if (abs(m1 - m2) <= threshold)
		return 1;
	return 0;
}


std::vector<Vec4i> filterLines(std::vector<Vec4i>  inputLines)
{

	std::vector<float> result;
	std::vector<float> mLines;
	std::vector<Vec4i>  outputLines;


	//salvez toate pantele intr-un vector
	for (auto line1 : inputLines)
	{
		float m;
		if (line1[2] - line1[0] != 0)
			m = 1.0 * (line1[3] - line1[1]) / (line1[2] - line1[0]);
		mLines.push_back(m);
	}


	for (size_t i = 0; i < mLines.size(); i++) {
		int cont = 0;
		for (size_t j = 0; j < mLines.size(); j++)
		{
			if (isParallel(mLines[i], mLines[j], 1.6) == 1 && i != j) {
				cont++;
				if (cont == 2) {
					result.push_back(mLines[i]);
					break;
				}
			}
		}
	}

	/*
	  for (auto m : mLines)
		  printf("%.3g ", m);
	  printf("\n");

	  for (auto m : result)
		  printf("%.3g ", m);
	  printf("\n");
	  */


	  // CONSTRUIREA LINIILOR
	for (auto line1 : inputLines) {
		float m;
		if ((line1[1] - line1[0]) == 0)
			m = 999;
		else
			m = 1.0 * (line1[3] - line1[2]) / (line1[1] - line1[0]);
		if (std::count(result.begin(), result.end(), m) > 0)
			outputLines.push_back(line1);
	}

	return outputLines;
}



std::vector<Vec4i> rangeOfSlopes(std::vector<Vec4i>  inputLines)
{

	std::vector<float> result;
	std::vector<float> mLines;
	std::vector<Vec4i>  outputLines;


	//salvez toate pantele intr-un vector
	for (auto line1 : inputLines)
	{
		float m;
		if (line1[1] - line1[0] == 0)
			m = 999;
		else
			m = 1.0 * (line1[3] - line1[2]) / (line1[1] - line1[0]);
		mLines.push_back(m);
	}


	for (auto m : mLines) {

		if (abs(m) > 0.2 && abs(m) < 4)
			result.push_back(m);
	}

	  // CONSTRUIREA LINIILOR
	for (auto line1 : inputLines) {
		float m;
		if ((line1[1] - line1[0]) == 0)
			m = 999;
		else
			m = 1.0 * (line1[3] - line1[2]) / (line1[1] - line1[0]);
		if (std::count(result.begin(), result.end(), m) > 0)
			outputLines.push_back(line1);
	}

	return outputLines;
}



void testCanny()
{

	char fname[MAX_PATH];

	while (openFileDlg(fname)) {
		Mat_<uchar> src = imread(fname, IMREAD_GRAYSCALE);
		Mat_<uchar> dst2Final;


		resize(src, src, Size(256, 256));

		//filtru gausian pentru eliminarea zgomotelor
		Mat blur;
		GaussianBlur(src, blur, Size(5, 5), 0); //in loc deroi_img i am pus src


		//detectare muchii cu canny
		Mat edges;
		Canny(blur, edges, 50, 150, 3);



		//salavare linii drepte cu HOUGHLINESP
		std::vector<Vec4i> lines2;
		HoughLinesP(edges, lines2, 1, CV_PI / 20 / 180, 80, 40, 200);


		//rafinez dreptele(pastrez doar cele paralele)
		// filterLines(lines2);


		//desenare linii
		Mat dst2 = drawLinesOnImageV2(src.rows, src.cols, lines2);

		Mat dst3 = drawLinesOnImageV2(src.rows, src.cols, rangeOfSlopes(lines2));



		//imshow("Canny", edges);
		imshow("HoughLinesV2", dst2);
		imshow("HoughLinesV3", dst3);

		//waitKey(0);
	}
}


int main()
{
	int op;
	do
	{
		system("cls");
		destroyAllWindows();
		printf("Menu:\n");
		printf(" 1 - Basic image opening...\n");
		printf(" 2 - Open BMP images from folder\n");
		printf(" 3 - Color to Gray\n");
		printf("\n PROIECT \n");
		printf(" 4 - Canny test\n");
		printf(" 0 - Exit\n\n");
		printf("Option: ");
		scanf("%d", &op);
		switch (op)
		{
		case 1:
			testOpenImage();
			break;
		case 2:
			testOpenImagesFld();
			break;
		case 3:
			testColor2Gray();
			break;
		case 4:
			testCanny();
			break;



		}
	} while (op != 0);
	return 0;
}