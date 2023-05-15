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

//calculeaza media noantelor de gri
int computeMean(Mat_<uchar> src)
{
	int s = 0;
	for (int i = 0; i < src.rows; i++)
		for (int j = 0; j < src.cols; j++)
			s += src(i, j);
	return s / (src.rows * src.cols);

}

//converteste din gray-scale in albnegru
Mat convertoBinary(Mat_<uchar> src) {

	Mat_<uchar> dst(src.cols, src.rows);

	int threshold = computeMean(src);

	for (int i = 0; i < src.rows; i++)
		for (int j = 0; j < src.cols; j++)
			if (src(i, j) <= threshold)
				dst(i, j) = 0;
			else
				dst(i, j) = 255;
	return dst;

}

//pasul1: gasesc prima linie(cea mai de sus)
//pasul 2: imi aleg latimea dintre linii
//pasul 3:cobor cu acea latime si generez restul dreptelor cu un for(cate drepte in total??)
//x1,y1,x2,y2  dreapta de start 
// se genereaza de sus in jos
//modelul se poate modifica din parametru
std::vector<Vec4i> generateIdealModel(int x1, int y1, int x2, int y2, int width, int nrDrepte)
{
	std::vector<Vec4i> crosswalk_lines;
	crosswalk_lines.push_back(Vec4i(x1, y1, x2, y2));

	for (int i = 1; i < nrDrepte; i++) {
		int currentWidth = i * width;
		crosswalk_lines.push_back(Vec4i(x1, y1 + currentWidth, x2, y2 + currentWidth));
	}

	//printf("\n size:  %d \n",crosswalk_lines.size());
	return crosswalk_lines;
}

//deseneaza modelul ideal de zebra alb-negru
Mat drawIdealModel(int rows, int cols, std::vector<Vec4i> crosswalk_lines)
{
	int border2 = 0, border1 = 0;
	int curr = 0;
	Mat_<uchar> dst = Mat::ones(rows, cols, CV_8UC1) * 255;

	for (auto line1 : crosswalk_lines) {
		border2 = line1[1]; //un y
		for (int i = border1; i < border2; i++)
			for (int j = 0; j < cols; j++)
				if (curr % 2 == 0)
					dst(i, j) = 0;

		border1 = border2;
		curr++;

	}

	for (int i = border2; i < rows; i++)
		for (int j = 0; j < cols; j++)
			dst(i, j) = 0;


	return dst;
}


//poate are acuratete mai mare cu generatePerspective???
//getPerspectiveTransform are nevoie de 2 vectori de 4 puncte fiecare
// punctele le obtin din vectorii dreptelor std::vector<Vec4i>(pentru test aleg primele 3 puncte) 
Mat generatePerspectiveTransformation(std::vector<Vec4i> idealModel, std::vector<Vec4i> zebraLines)
{
	int zebraLen = zebraLines.size();
	int idealLen = zebraLines.size();
	if (zebraLen < 1)
		return cv::Mat::zeros(256, 256, CV_8UC1);

	Point2f srcTri[4];
	srcTri[0] = Point2f(idealModel[0][0] * 1.0, idealModel[0][1] * 1.0);
	srcTri[1] = Point2f(idealModel[0][2] * 1.0, idealModel[0][3] * 1.0);
	srcTri[2] = Point2f(idealModel[1][0] * 1.0, idealModel[1][1] * 1.0);
	srcTri[3] = Point2f(idealModel[1][2] * 1.0, idealModel[1][3] * 1.0);

	Point2f dstTri[4];
	dstTri[0] = Point2f(zebraLines[2][0] * 1.0, zebraLines[2][1] * 1.0);
	dstTri[1] = Point2f(zebraLines[2][2] * 1.0, zebraLines[2][3] * 1.0);
	dstTri[2] = Point2f(zebraLines[3][0] * 1.0, zebraLines[3][1] * 1.0);
	dstTri[3] = Point2f(zebraLines[3][2] * 1.0, zebraLines[3][3] * 1.0);

	return getPerspectiveTransform(srcTri, dstTri);
}

//calculeaza diferenta de negru-alb a zebrei proiectate (ca numar de stripe-uri) 
int countBlackWhites(Mat_<uchar> zebraProjected)
{
	int area = 0,ci=0,ri=0;
	int nrStripes = 0;
	//area
	int rows = zebraProjected.rows;
	int cols = zebraProjected.cols;
	for (int i = 0; i < rows; i++)
		for (int j = 0; j < cols; j++)
			if (zebraProjected(i, j) == 255)
				area++;

	for (int i = 0; i < zebraProjected.rows; i++)
		for (int j = 0; j < zebraProjected.cols; j++)
		{
			if (zebraProjected(i, j) == 255) {
				ci += j;
				ri += i;
			}
			   
		}
	ri /= area;

	ci /= area;
	int intra = 0;
	for (int y = 1; y < rows; y++)
		if (zebraProjected(ci, y) == 255 && zebraProjected(ci, y-1) == 0)
			if (intra == 0){
				nrStripes++;
				intra = 1;
			}
			else { intra = 0; }
				
	return nrStripes;
}

//calculeaza aria mastii albe
int checkWhiteArea(Mat_<uchar> whiteMask,int downLimit,int upLimit)
{
	int area = 0;
	for (int i = 0; i < whiteMask.rows; i++)
		for (int j = 0; j < whiteMask.cols; j++)
			if (whiteMask(i, j) == 255)
				area++;
	if (downLimit < area && area < upLimit)
		return 1;
	return 0;
}

// RMSE
int generateRMSE(Mat_<uchar> src1, Mat_<uchar> whiteMask, Mat_<uchar> zebraProjected)
{

	int rows = src1.rows;
	int cols = src1.cols;
	int sum = 0;

	//trebuie sa modific count??
	int count = rows * cols;
	//int count = countWhites(whiteMask);

	for (int i = 0; i < src1.rows; i++)
		for (int j = 0; j < src1.cols; j++) {
			//calculez doar pentru cele din masca
			if (whiteMask(i, j) == 255) {

				int pixelDiff = src1.at<uchar>(i, j) - zebraProjected(i, j);
				sum += pixelDiff * pixelDiff;
			}
		}

	return  sum / count;

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


Mat RANSAC1(Mat_<uchar> src, std::vector<Vec4i> filteredLines, std::vector<Vec4i> idealModel) {
	Point2f dstTri[4];
	Point2f srcTri[4];
	int m, n;//dreptele din filteredLines
	int size = filteredLines.size();
	int best_rmse = 99999;
	Mat best_matrix = cv::Mat::zeros(3, 3, CV_32FC1);



	//masca de alb
	Mat white_mask = Mat::ones(src.rows, src.cols, src.type());

	//imagine full alb
	Mat white = Mat::ones(src.rows, src.cols, src.type()) * 255;

	//zebra proiectata
	Mat warp_dst = Mat::ones(src.rows, src.cols, src.type());

	//generez modelul ideal
	Mat idealModelDrawed = drawIdealModel(src.rows, src.cols, idealModel);

	//primele doua drepte din modelul ideal
	srcTri[0] = Point2f(idealModel[0][0] * 1.0, idealModel[0][1] * 1.0);
	srcTri[1] = Point2f(idealModel[0][2] * 1.0, idealModel[0][3] * 1.0);
	srcTri[2] = Point2f(idealModel[1][0] * 1.0, idealModel[1][1] * 1.0);
	srcTri[3] = Point2f(idealModel[1][2] * 1.0, idealModel[1][3] * 1.0);

	dstTri[0] = Point2f(0, 0);
	dstTri[1] = Point2f(0, 0);
	dstTri[2] = Point2f(0, 0);
	dstTri[3] = Point2f(0, 0);


	for (m = 0; m < size/2; m++)
		for (n = 0; n < size/2; n++)
			if (m != n && (m+n)%2==1 && abs(m-n)<3) {

				//float m1 = 1.0 * (filteredLines[m][3] - filteredLines[m][1]) / (filteredLines[m][2] - filteredLines[m][0]);
				//float m2 = 1.0 * (filteredLines[n][3] - filteredLines[n][1]) / (filteredLines[n][2] - filteredLines[n][0]);

				

		             //asocierile pentru liniile detectate
					dstTri[0] = Point2f(filteredLines[m][0] * 1.0, filteredLines[m][1] * 1.0);
					dstTri[1] = Point2f(filteredLines[m][2] * 1.0, filteredLines[m][3] * 1.0);
					dstTri[2] = Point2f(filteredLines[n][0] * 1.0, filteredLines[n][1] * 1.0);
					dstTri[3] = Point2f(filteredLines[n][2] * 1.0, filteredLines[n][3] * 1.0);

					//generez o matrice perspectiva
					Mat currenMatrix = getPerspectiveTransform(srcTri, dstTri);

					//generez masca alba proiectata
					warpPerspective(white, white_mask, currenMatrix, warp_dst.size());

					//generez zebra proiectata
					warpPerspective(idealModelDrawed, warp_dst, currenMatrix, warp_dst.size());

					int currentRMSE = generateRMSE(src, white_mask, warp_dst);



					printf(" RMSE %d\n", currentRMSE);
					if (currentRMSE < best_rmse) {
						best_rmse = currentRMSE;
						best_matrix = currenMatrix;

					}
				

			}

	return best_matrix;
}


//deseneaza poza filana : proiectia zebrei ideale peste imaginea initiala
Mat drawFinalCrossWalk(Mat_<uchar> src, Mat_<uchar> whiteMask, Mat_<uchar> zebraProjected)
{
	Mat_<uchar> dst(src.rows, src.cols, 255);

	for (int i = 0; i < src.rows; i++)
		for (int j = 0; j < src.cols; j++) {
			//calculez doar pentru cele din masca
			if (whiteMask(i, j) == 255) {
				dst(i, j) = zebraProjected(i, j);
			}
			else {
				dst(i, j) = src(i, j);
			}
		}
	return dst;
}


void testZebre()
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
		HoughLinesP(edges, lines2, 1, CV_PI / 180 / 10, 80, 40, 30);



		//rafinez dreptele(pastrez doar cele paralele)
		std::vector<Vec4i> filteredLines = filterLines(lines2);


		//generare matrice de linii
		Mat dst2 = drawLinesOnImageV2(src.rows, src.cols, lines2);

		Mat dst3 = drawLinesOnImageV2(src.rows, src.cols, filteredLines);


		//testare transformata perspectiva
		////////////////////////////////////////////       x1 y1  x2  y2  wd nrDrepte 
		std::vector<Vec4i> idealModel = generateIdealModel(0, 4, 255, 4, 35, 8);

		Mat idealModelDrawed = drawIdealModel(src.rows, src.cols, idealModel);

		Mat warp_matrix = RANSAC1(src, filteredLines, idealModel);  //generatePerspectiveTransformation(idealModel, filteredLines);


		//afisez matricea  transformatei afine
		//printTransformationMatrix(warp_matrix);


		//zebra proiectata
		Mat warp_dst = Mat::ones(src.rows, src.cols, src.type());

		//masca de alb
		Mat white_mask = Mat::ones(src.rows, src.cols, src.type());

		//imagine full alb
		Mat white = Mat::ones(src.rows, src.cols, src.type()) * 255;

		warpPerspective(idealModelDrawed, warp_dst, warp_matrix, warp_dst.size());
		warpPerspective(white, white_mask, warp_matrix, warp_dst.size());

		//afisez un rmse fara ransac
		//int rmse = generateRMSE(src, white_mask, warp_dst);
		//printf("RMSE %d \n", rmse); 
		printf(" STRIPES %d \n", countBlackWhites( warp_dst));
		

		//imshow("Canny", edges);
		imshow("WhiteMask", white_mask);
		imshow("Perspective", warp_dst);
		imshow("Initial", src);
		imshow("FINAL", drawFinalCrossWalk(src, white_mask, warp_dst));
		//imshow("HoughLines", dst2);
		imshow("Filtered", dst3);
		//waitKey(0);
	}
}


void testIdealModel() {

	////////////////////////////////////////////  x1 y1  x2 y2 wd len 
	std::vector<Vec4i> lines = generateIdealModel(0, 4, 255, 4, 35, 8);
	Mat dst = drawLinesOnImageV2(256, 256, lines);
	Mat dst2 = drawIdealModel(256, 256, lines);

	imshow("ideal model", dst);
	imshow("ideal model drawed", dst2);

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
		printf(" 4 - Zebre\n");
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
			testZebre();
			break;
		case 5:
			testIdealModel();
			break;




		}
	} while (op != 0);
	return 0;
}