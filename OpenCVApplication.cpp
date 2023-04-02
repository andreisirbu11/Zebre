// OpenCVApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "common.h"


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
	if (p.x < rows &&  p.y < cols)
		return 1;
	return 0;
}

Mat drawLinesOnImage( int rows,int cols,std::vector<Vec2f> lines) {
	Mat_<uchar> dst(rows,cols);

	for (int i = 0; i < rows; i++)
		for (int j = 0; j < cols; j++)
			dst(i, j) = 0;

	// Draw the lines on the original image
	for (auto line : lines) {
		float rho = line[0];
		float theta = line[1];
		double a = cos(theta), b = sin(theta);
		double x0 = a * rho, y0 = b * rho;
		cv::Point pt1(cvRound(x0 + 800 * (-b)), cvRound(y0 + 800 * (a)));
		cv::Point pt2(cvRound(x0 - 800 * (-b)), cvRound(y0 - 800 * (a)));

        cv::line(dst, pt1, pt2,255, 1);
	}

	return dst;
}

void testCanny()
{

	char fname[MAX_PATH];
	
	while (openFileDlg(fname)) {
		Mat_<uchar> src = imread(fname, IMREAD_GRAYSCALE);
		
		 
		//regiunea de interes ROI   MAI AM NEVOIE DE ROI????????

		int x =  50;  // x  punctul din stanga sus a lui ROI
		int y =  50;  // y  punctul din stanga sus a lui ROI
		int width =  150; // latime ROI
		int height =  100; // inaltime ROI

		Rect roi(x, y, width, height); 
		Mat roi_img = src(roi);

		//filtru gausian pentru eliminarea zgomotelor
		Mat blur;
		GaussianBlur(src, blur, Size(5, 5), 0); //in loc deroi_img i am pus src

		Mat edges;
		Canny(blur, edges, 50, 150 , 3);


		// Standard Hough Line Transform
		std::vector<Vec2f> lines; // liniile selectate
		HoughLines(edges, lines, 1, CV_PI/4 / 180,100,0, 0); 

		//imaginea finala 
		Mat dst = drawLinesOnImage(src.rows, src.cols, lines);


		/*
		//resize pentru edges (optional)
		Mat edgesFinal;
		resize(edges, edgesFinal, Size(256, 256), 0, 0, INTER_LINEAR);
		*/

		imshow("before", src);
		imshow("canny", edges);
		imshow("HoughLines", dst);
		waitKey(0);
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