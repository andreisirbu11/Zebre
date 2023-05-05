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
////***************************** PROJECT *********************************//////   
/////////////////////////////////////////////////////////////////////////////////


int isInside(cv::Point p, int rows, int cols) {
	if (p.x < rows && p.y < cols)
		return 1;
	return 0;
}


//pasul1: gasesc prima linie(cea mai de sus)
//pasul 2: imi aleg latimea dintre linii
//pasul 3:cobor cu acea latime si generez restul dreptelor cu un for(cate drepte in total??)
//x1,y1,x2,y2  dreapta de start 
// se genereaza de sus in jos
//modelul se poate modifica din parametru
std::vector<Vec4i> generateIdealModel(int x1,int y1,int x2,int y2, int width,int nrDrepte)
{
	std::vector<Vec4i> crosswalk_lines;
	crosswalk_lines.push_back(Vec4i(x1,y1,x2,y2));

	for(int i=1;i<=nrDrepte;i++){
	   int currentWidth = i * width;
	   crosswalk_lines.push_back(Vec4i(x1, y1 + currentWidth, x2, y2 + currentWidth));
	}
	
	//printf("\n size:  %d \n",crosswalk_lines.size());
	return crosswalk_lines;
}

// getAffineTransform are nevoie de 2 vectori de 3 puncte fiecare
// punctele le obtin din vectorii dreptelor std::vector<Vec4i>(pentru test aleg primele 3 puncte) 
Mat generateAffineTransformation(std::vector<Vec4i> idealModel, std::vector<Vec4i> zebraLines)
{

	Point2f srcTri[3];
	srcTri[0] = Point2f(idealModel[0][0]*1.0,idealModel[0][1] * 1.0);
	srcTri[1] = Point2f(idealModel[0][2] * 1.0, idealModel[0][3] * 1.0);
	srcTri[2] = Point2f(idealModel[1][0] * 1.0, idealModel[1][1] * 1.0);

	Point2f dstTri[3];
	dstTri[0] = Point2f(zebraLines[0][0] * 1.0, zebraLines[0][1] * 1.0);
	dstTri[1] = Point2f(zebraLines[0][2] * 1.0, zebraLines[0][3] * 1.0);
	dstTri[2] = Point2f(zebraLines[1][0] * 1.0, zebraLines[1][1] * 1.0);

	return getAffineTransform(srcTri, dstTri);
}

//poate are acuratete mai mare cu generatePerspective???
//getPerspectiveTransform are nevoie de 2 vectori de 4 puncte fiecare
// punctele le obtin din vectorii dreptelor std::vector<Vec4i>(pentru test aleg primele 3 puncte) 
Mat generatePerspectiveTransformation(std::vector<Vec4i> idealModel, std::vector<Vec4i> zebraLines)
{
	if (zebraLines.size() < 1)
		return cv::Mat::zeros(256, 256, CV_8UC1);
	Point2f srcTri[4];
	srcTri[0] = Point2f(idealModel[0][0] * 1.0, idealModel[0][1] * 1.0);
	srcTri[1] = Point2f(idealModel[0][2] * 1.0, idealModel[0][3] * 1.0);
	srcTri[2] = Point2f(idealModel[1][0] * 1.0, idealModel[1][1] * 1.0);
	srcTri[3] = Point2f(idealModel[1][2] * 1.0, idealModel[1][3] * 1.0);

	Point2f dstTri[4];
	dstTri[0] = Point2f(zebraLines[0][0] * 1.0, zebraLines[0][1] * 1.0);
	dstTri[1] = Point2f(zebraLines[0][2] * 1.0, zebraLines[0][3] * 1.0);
	dstTri[2] = Point2f(zebraLines[1][0] * 1.0, zebraLines[1][1] * 1.0);
	dstTri[3] = Point2f(zebraLines[1][2] * 1.0, zebraLines[1][3] * 1.0);

	return getPerspectiveTransform(srcTri, dstTri);
}



void printTransformationMatrix(Mat warp_mat)
{
	for (int i = 0; i < warp_mat.rows; i++)
	{
		for (int j = 0; j < warp_mat.cols; j++)
			printf("%.6f ", warp_mat.at<double>(i, j));
		printf("\n");
	}
	printf("\n");
}

Mat drawLinesOnImageV2(int rows, int cols, std::vector<Vec4i> lines) {
	Mat_<uchar> dst(rows, cols, 255);
	for (size_t i = 0; i < lines.size(); i++)
	{
		line(dst, Point(lines[i][0], lines[i][1]),
			Point(lines[i][2], lines[i][3]), 0, 1);
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
		{ //0.03
			if (isParallel(mLines[i], mLines[j], 0.03) == 1 && i != j) {
				cont++;
				if (cont == 5) {
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
		if (line1[2] - line1[0] != 0) {

			m = 1.0 * (line1[3] - line1[1]) / (line1[2] - line1[0]);

			if (std::count(result.begin(), result.end(), m) > 0)
				outputLines.push_back(line1);
		}
	}

	return outputLines;
}


void testCanny()
{

	char fname[MAX_PATH];

	while (openFileDlg(fname)) {
		Mat_<uchar> src = imread(fname, IMREAD_GRAYSCALE);
	
		resize(src, src, Size(256, 256));

		//filtru gausian pentru eliminarea zgomotelor
		Mat blur;
		GaussianBlur(src, blur, Size(5, 5), 0); //in loc deroi_img i am pus src


		//detectare muchii cu canny
		Mat edges;
		Canny(blur, edges, 50, 150, 3);



		//salavare linii drepte cu HOUGHLINESP
		std::vector<Vec4i> lines2;
		//HoughLinesP(edges, lines2, 1, CV_PI / 20 / 180, 80, 40, 200);
		HoughLinesP(edges, lines2, 1, CV_PI/180/10, 80, 40, 30);
		


		//rafinez dreptele(pastrez doar cele paralele)
		std::vector<Vec4i> filteredLines= filterLines(lines2);

		

		//generare matrice de linii
		Mat dst2 = drawLinesOnImageV2(src.rows, src.cols, lines2);

		Mat dst3 = drawLinesOnImageV2(src.rows, src.cols, filteredLines);

		//testare transformata afina
		std::vector<Vec4i> idealModel = generateIdealModel(20, 20, 230, 20, 20, 10);
		Mat dst4 = drawLinesOnImageV2(src.rows, src.cols, idealModel);

		Mat warp_mat = generatePerspectiveTransformation(idealModel, filteredLines);

		//afisez matricea  transformatei afine

		printTransformationMatrix(warp_mat);

		//aplicarea transformatei pe imaginea sursa
		Mat warp_dst = Mat::zeros(src.rows, src.cols, src.type());
		
		warpPerspective(dst4, warp_dst, warp_mat, warp_dst.size());


		//imshow("Canny", edges);
		imshow("Affine", warp_dst);
		imshow("Initial", src);
		imshow("HoughLines", dst2);
		imshow("Filtered", dst3);

		//waitKey(0);
	}
}

void testIdealModel() {

	////////////////////////////////////////////  x1 y1  x2 y2 wd len 
	std::vector<Vec4i> lines = generateIdealModel(20,20,230,20,20,10);
	Mat dst = drawLinesOnImageV2(256, 256, lines);

	imshow("ideal model", dst);

	waitKey(0);
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
		printf(" 5 - Ideal Model test\n");
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
		case 5:
			testIdealModel();
			break;



		}
	} while (op != 0);
	return 0;
}