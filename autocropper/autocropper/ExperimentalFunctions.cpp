#include "ExperimentalFunctions.h"
#include "FileUtilities.h"
#include "OcvUtilities.h"
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>

using namespace cv;
using namespace experimental;
using namespace std;
using namespace OcvUtility;

namespace experimental
{
	//////////////////////////////////////////////////////////////////////////////////
	// computeGradientImage()
	//
	// Compute the gradient of the specified image and return it.
	// Code from:
	// http://docs.opencv.org/2.4.10/doc/tutorials/imgproc/imgtrans/sobel_derivatives/sobel_derivatives.html
	//////////////////////////////////////////////////////////////////////////////////
	Mat computeGradientImage(Mat image)
	{
		int scale = 1;
		int delta = 0;
		int ddepth = CV_8UC1;
		Mat grad;

		/// Generate grad_x and grad_y
		Mat grad_x, grad_y;
		Mat abs_grad_x, abs_grad_y;

		/// Gradient X
		//Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );
		Sobel(image, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT);
		convertScaleAbs(grad_x, abs_grad_x);

		/// Gradient Y
		//Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
		Sobel(image, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT);
		convertScaleAbs(grad_y, abs_grad_y);

		/// Total Gradient (approximate)
		addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);

		return grad;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// drawRedRectOnImage()
	//
	// Draw a red rectangle on the image.
	//////////////////////////////////////////////////////////////////////////////////
	Mat drawRedRectOnImage(Mat image, Rect rect, int thickness)
	{
		Mat convertedImage = image.clone();

		cvtColor(image, convertedImage, CV_GRAY2RGB);
		rectangle(convertedImage, rect, Scalar(0, 0, 255), thickness);

		return convertedImage;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// computeInnermostRectangle()
	//
	// Compute the innermost rectangle that can be defined based on the specified
	// image.
	//////////////////////////////////////////////////////////////////////////////////
	Rect computeInnermostRectangle(Mat image)	//TODO: This function name is terrible.
	{
		Point center = Point(image.size().width / 2, image.size().height / 2);
		int u = 0, d = image.size().height, l = 0, r = image.size().width;

		// Find the nearest nonwhite pixel above.
		for (int i = center.y; i >= 0; --i)
		{
			Point currentPoint = Point(center.x, i);
			int valueAtCurrentPoint = image.at<uchar>(currentPoint);
			if (valueAtCurrentPoint != 0)
			{
				u = i;
				break;
			}
		}

		// Find the nearest nonwhite pixel below.
		for (int i = center.y; i < image.size().height; ++i)
		{
			Point currentPoint = Point(center.x, i);
			if (image.at<uchar>(currentPoint) != 0)
			{
				d = i;
				break;
			}
		}

		// Find the nearest nonwhite pixel to the left.
		for (int i = center.x; i >= 0; --i)
		{
			Point currentPoint = Point(i, center.y);
			if (image.at<uchar>(currentPoint) != 0)
			{
				l = i;
				break;
			}
		}

		// Find the nearest nonwhite pixel to the right.
		for (int i = center.x; i < image.size().width; ++i)
		{
			Point currentPoint = Point(i, center.y);
			if (image.at<uchar>(currentPoint) != 0)
			{
				r = i;
				break;
			}
		}

		return Rect(l, u, r - l, d - u);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// computeGelLocation()
	//
	// Find the rectangle corresponding to the gel location within the image.
	// 
	// TODO: Don't assume that the gel has been properly segmented before being passed
	//		 in.
	//////////////////////////////////////////////////////////////////////////////////
	Rect computeGelLocation(Mat image)
	{
		// Find the top, bottom, left, right pixels of the contour. This is basically the first nonblack pixel that we find by doing a line scan.

		int leftMost = image.size().width - 1, rightMost = 0, topMost = image.size().height - 1, bottomMost = 0;

		for (int i = 0; i < image.size().width; ++i)
		{
			for (int j = 0; j < image.size().height; ++j)
			{
				Point currentPoint = Point(i, j);
				uchar currentValue = image.at<uchar>(currentPoint);
				
				if (currentValue != 0)
				{
					if (currentPoint.x < leftMost)
						leftMost = currentPoint.x;

					if (currentPoint.x > rightMost)
						rightMost = currentPoint.x;

					if (currentPoint.y < topMost)
						topMost = currentPoint.y;

					if (currentPoint.y > bottomMost)
						bottomMost = currentPoint.y;
				}
			}
		}

		return Rect(leftMost, topMost, rightMost - leftMost, bottomMost - topMost);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// findLargestHorizontalLines()
	//
	// Returns an image containing the largest horizontal lines found in the image.
	//////////////////////////////////////////////////////////////////////////////////
	Mat findLargestHorizontalLines(Mat image)
	{
		Mat horizontalLines;

		auto horizElem = getStructuringElement(MORPH_RECT, Size(image.size().width / 2, 1));
		morphologyEx(image, horizontalLines, MORPH_OPEN, horizElem);

		return horizontalLines;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// findLargestVerticalLines()
	//
	// Returns an image containing the largest vertical lines found in the image.
	//////////////////////////////////////////////////////////////////////////////////
	Mat findLargestVerticalLines(Mat image)
	{
		Mat verticalLines;

		auto vertElem = getStructuringElement(MORPH_RECT, Size(1, image.size().height / 2));
		morphologyEx(image, verticalLines, MORPH_OPEN, vertElem);

		return verticalLines;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// computeForegroundImage()
	//
	// Computes a foreground image based on some background subtraction method.
	//////////////////////////////////////////////////////////////////////////////////
	Mat computeForegroundImage(const vector<Mat>& images)
	{
		Mat foregroundMask, foregroundImage, backgroundImage;
		Ptr<BackgroundSubtractorMOG2> pMOG2 = createBackgroundSubtractorMOG2();

		vector<Mat> foregroundMasks;

		int i = 0;

		for each (Mat image in images)
		{
			if (foregroundImage.empty())
				foregroundImage.create(image.size(), image.type());

			pMOG2->apply(image, foregroundMask);

			foregroundImage = Scalar::all(0);
			image.copyTo(foregroundImage, foregroundMask);

			//string filename = utility::FileUtilities::buildFilename("C:\\Temp\\images\\fg", ++i);
			//imwrite(filename, foregroundImage);
		}

		return foregroundImage;

		waitKey();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// computeForegroundImages()
	//
	// Computes a foreground images based on some background subtraction method.
	//////////////////////////////////////////////////////////////////////////////////
	vector<Mat> computeForegroundImages(const vector<Mat>& images)
	{
		Mat foregroundMask, foregroundImage, backgroundImage;
		Ptr<BackgroundSubtractorMOG2> pMOG2 = createBackgroundSubtractorMOG2();

		vector<Mat> foregroundImages;

		int i = 0;

		for each (Mat image in images)
		{
			if (foregroundImage.empty())
				foregroundImage.create(image.size(), image.type());

			pMOG2->apply(image, foregroundMask);

			foregroundImage = Scalar::all(0);
			image.copyTo(foregroundImage, foregroundMask);	//TODO: Does using the mask rather than the image improve the results?

			string filename = utility::FileUtilities::buildFilename("C:\\Temp\\images\\fg", ++i);
			if (i > 1)	//TODO_DR: Deal with the first file.
			{
				imwrite(filename, foregroundImage);
				foregroundImages.push_back(foregroundImage.clone());
			}
		}

		return foregroundImages;

		waitKey();
	}

	//////////////////////////////////////////////////////////////////////////////////
	// computeHistogram()
	//
	// Compute the histogram of the specified image and return it.
	//////////////////////////////////////////////////////////////////////////////////
	cv::Mat computeHistogram(cv::Mat image)
	{
		Mat histogram;

		int numBins = 256;

		float range[] = { 0, 256 };
		const float* histogramRange = { range };
		bool uniform = true;

		calcHist(&image, 1, 0, Mat(), histogram, 1, &numBins, &histogramRange);

		// Draw histogram.
		int windowWidth = 1024; int windowHeight = 800;
		int bin_w = cvRound((double)windowWidth / numBins);

		Mat histImage(windowHeight, windowWidth, CV_8UC1, Scalar(0, 0, 0));

		/// Normalize the result to [ 0, histImage.rows ]
		normalize(histogram, histogram, 0, histImage.rows, NORM_MINMAX, -1, Mat());

		for (int i = 1; i < numBins; i++)
		{
			line(histImage, Point(bin_w*(i - 1), windowHeight - cvRound(histogram.at<float>(i - 1))),
				Point(bin_w*(i), windowHeight - cvRound(histogram.at<float>(i))),
				Scalar(255, 255, 255));
		}

		return histImage;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// plotHistogram()
	//
	// Another method to compute the histogram.
	//////////////////////////////////////////////////////////////////////////////////
	Mat plotHistogram(Mat image)
	{
		const unsigned int NUMBER_OF_BINS = 256;
		const unsigned int WINDOW_HEIGHT = NUMBER_OF_BINS;
		const unsigned int WINDOW_WIDTH = NUMBER_OF_BINS;
		Mat histogramImage = Mat::zeros(WINDOW_HEIGHT, WINDOW_WIDTH, CV_8UC1);

		double hist[NUMBER_OF_BINS] = { 0 };

		// Let's compute the histogram.
		MatIterator_<uchar> it, end;
		for (it = image.begin<uchar>(), end = image.end<uchar>();
			it != end;
			++it)
		{
			hist[*it]++;
		}

		// Let's find the max bin amount in the histogram, so that we can scale the rest of the histogram accordingly.
		double max = 0;
		for (unsigned int bin = 0; bin < NUMBER_OF_BINS; ++bin)
		{
			const double binValue = hist[bin];
			if (binValue > max)
				max = binValue;
		}

		// Let's plot the histogram.
		for (unsigned int bin = 0; bin < NUMBER_OF_BINS; ++bin)
		{
			const int binHeight = static_cast<int>(hist[bin] * WINDOW_HEIGHT / max);

			line(histogramImage, Point(bin, WINDOW_HEIGHT - binHeight), Point(bin, WINDOW_HEIGHT), Scalar(255, 255, 255));
		}

		return histogramImage;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// generateEnhancedCenterMask()
	//
	// Generate a mask which has a value of 1.0 in the center, and 0.0 along the
	// edges, with a smooth gradient between. This mask may be sensitive to 
	// variations between length and width of the image.
	//////////////////////////////////////////////////////////////////////////////////
	Mat generateEnhancedCenterMask(Size size)
	{
		Mat image = Mat::ones(size, CV_8UC1);
		padImage(image, image);
		distanceTransform(image, image, CV_DIST_C, 3);
		double maxVal;
		minMaxLoc(image, NULL, &maxVal);
		image *= 1/maxVal;

		removePadding(image, image);

		return image;
	}
}
