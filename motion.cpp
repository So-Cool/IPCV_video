#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <cassert>
#include <iostream>

#include "motion.hpp"

// LKTracker constructor
LKTracker::LKTracker() 
	: magnitude_treshold(30.0)
{
}

LKTracker::~LKTracker()
{
	// Update all tracked region
	for(MotionVector::iterator iter = this->regions.begin(); iter != this->regions.end(); ++iter)
	{
		Motion *motion = *iter;

		if(motion != NULL)
		{
			delete motion;
			motion = NULL;
		}
	}
	this->regions.clear();
}

// Add region to tracking
void LKTracker::AddRegion(cv::Vec2i position, cv::Size regionSize, cv::Mat& frame, cv::Mat& next)
{
	Motion * motionRegion = new Motion(position, regionSize, frame, next);
	this->regions.push_back(motionRegion);

	int num_regions = regions.size();

	std::stringstream xStream, yStream, tStream;
	xStream << num_regions << " X derivative";
	yStream << num_regions << " Y derivative";
	tStream << num_regions << " T derivative";

	std::string x, y, t;
	x = xStream.str();
	y = yStream.str();
	t = tStream.str();

	//cv::namedWindow(x, 2);
	//cv::namedWindow(y, 2);
	//cv::namedWindow(t, 2);

	motionRegion->SetWindowNames(x, y, t);
}

// Update with new frames
void LKTracker::Update(cv::Mat& frame, cv::Mat& next)
{
	MotionVector::iterator iter;

	// Update all tracked region
	for(iter = this->regions.begin(); iter != this->regions.end(); ++iter)
	{
		Motion *motionRegion = *iter;
		motionRegion->Update(frame, next);
	}
}

void LKTracker::ShowAll()
{
	MotionVector::iterator iter;

	// For each region, show derivatives
	for(iter = this->regions.begin(); iter != this->regions.end(); ++iter)
	{
		Motion *motion = (*iter);

		cv::Mat a, b, c;
		motion->getIx().convertTo(a, CV_8U);
		motion->getIy().convertTo(b, CV_8U);
		motion->getIt().convertTo(c, CV_8U);
		cv::normalize(a, a, 0, 255, cv::NORM_MINMAX);
		cv::normalize(b, b, 0, 255, cv::NORM_MINMAX);
		cv::normalize(c, c, 0, 255, cv::NORM_MINMAX);

		cv::imshow(motion->getWindowTitleX(), a);
		cv::imshow(motion->getWindowTitleY(), b);
		cv::imshow(motion->getWindowTitleT(), c);
	}
}

void LKTracker::ShowMotion(cv::Mat& image)
{
	MotionVector::iterator iter;

	// TRUE- 1 is Right 	|	 FALSE- 0 is Left
	int motionR, motionL, motionLast, motionLongestL, motionLongestR;
	double motionSumL, motionSumR;

	for(iter = this->regions.begin(); iter != this->regions.end(); ++iter)
	{
		Motion *motion = (*iter);

		motionR = 0;
		motionL = 0;
		motionLast = 0;
		motionLongestR = 0;
		motionLongestL = 0;
		motionSumL = 0;
		motionSumR = 0;

		cv::Rect origin = motion->getRect();

		cv::rectangle(image, origin, CV_RGB(255, 0, 255), 2);

		for(int i = 0; i < motion->getVx().rows; i++)
		{
			for(int j = 0; j < motion->getVx().cols; j++)
			{
				double x_component = motion->getVx().at<double>(i,j);
				double y_component = motion->getVy().at<double>(i,j);

				//std::cout << "vx " << x_component << " vy " << y_component << std::endl;

				cv::Point p1 = cv::Point(j+origin.x, i+origin.y);
				cv::Point p2 = cv::Point(j+x_component+origin.x, i + y_component+origin.y);

				// distance ?????
				// if(cv::norm(px1-px2) < magnitude_treshold || cv::norm(py1-py2) < magnitude_treshold)
				
				if(cv::norm(p1-p2) >= magnitude_treshold)
				{
					// Draw the vector
					cv::circle ( image , p1 , 4 , cv::Scalar(0,255,0) , 2 , 8 );
					cv::line(image, p1, p2, CV_RGB(255, 0, 0), 2);
					cv::circle ( image , p2 , 1 , cv::Scalar(0,255,0) , 2 , 8 );
				}

				if(cv::norm(p1-p2) < 5)
				{
					continue;
				}

				if(detectMotion(p1, p2) == 1) {
					motionSumR += std::abs(x_component);
					motionR++;
				} else if(detectMotion(p1, p2) == 0) {
					motionSumL += std::abs(x_component);
					motionL++;
				}

				/*
				// detect gestures
				if(detectMotion(p1, p2) == 1 && motionLast == 1)
				{
					++motionR;
					motionLast = 1;
				}
				else if (detectMotion(p1, p2) == 1 && motionLast == 0)
				{
					motionR = 1;
					motionLast = 1;

					// check if last sequence of LEFTS- 0 was longest one
					if (motionLongestL < motionL)
					{
						motionLongestL = motionL;
					}
				}
				else if(detectMotion(p1, p2) == 0 && motionLast == 0)
				{
					++motionL;
					motionLast = 0;
				}
				else if(detectMotion(p1, p2) == 0 && motionLast == 1)
				{
					motionL = 1;
					motionLast = 0;

					// check if last sequence of RIGHTS- 1 was longest one
					if (motionLongestR < motionR)
					{
						motionLongestR = motionR;
					}
				}
				*/
				
			}
		}

		// print detected motion
		/*if (motionLongestR>motionLongestL)
			std::cout << "RIGHT" << std::endl;
		else if (motionLongestR<motionLongestL)
			std::cout << "LEFT" << std::endl;

			*/
		double av_motion_l = motionSumL / motionL;
		double av_motion_r = motionSumR / motionR;

		cv::Point p(origin.x+50, origin.y+50);
		if (av_motion_r>av_motion_l) {
			cv::putText(image, "R", p, cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0,255,0), 3);
		}
		else if (av_motion_r<av_motion_l) {
			cv::putText(image, "L", p, cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0,255,0), 3);
		}
	}
}

int LKTracker::detectMotion(cv::Point A, cv::Point B)
{

	// check dominant motion; if it's in Y direction disregard
	if(abs(B.y-A.y) > abs(B.x-A.x))
		return -1;

	//majority movement in a single sequence


	// disregard motion in Y
	// if (abs(A.y-B.y) < 20)
	// {

		if (A.x-B.x > 5)
		{
			// std::cout << "RIGHT" << std::endl;
			return 1;
		}

		if (A.x-B.x < -5)
		{
			// std::cout << "LEFT" << std::endl;
			return 0;
		}
	// }

	return -1;
}

Motion::Motion(cv::Vec2i position, cv::Size regionSize, cv::Mat& frame, cv::Mat& next)
{
	int x = position[0];
	int y = position[1];

	// Check if this region is possible
	assert(x >= 0 && y >= 0 && x < frame.cols && y < frame.rows);

	// Make sure regions are not too big
	if(x + regionSize.width >= frame.cols)
	{
		regionSize.width = frame.cols - x;
	}

	if(y+regionSize.height >= frame.rows)
	{	
		regionSize.height = frame.rows - y;
	}
	
	std::cout << "Created region from x: " << x << " y: " << y <<
		" width: " << regionSize.width << " height: " << regionSize.height << std::endl;
	// Create rectangle to extract region later
	this->region = cv::Rect(x, y, regionSize.width, regionSize.height);

	// Initialize derivative
	this->derivative = new Derivative(this->region.height, this->region.width);	
}

Motion::~Motion()
{
	if(this->derivative != NULL)
	{
		delete this->derivative;
		this->derivative = NULL;
	}
}

void Motion::Update(cv::Mat& frame, cv::Mat& next)
{
	this->extractRegionAndUpdate(frame, next);
	this->derivative->computeVelocity();
}

void Motion::extractRegionAndUpdate(cv::Mat& frame, cv::Mat& next)
{
	this->extractedFrame = frame(this->region);
	this->extractedNext = next(this->region);

	this->derivative->setDerivatives(extractedFrame, extractedNext);
}

void Motion::SetWindowNames(std::string& xWindowTitle, std::string& yWindowTitle, std::string& tWindowTitle)
{
	this->xWindowTitle = xWindowTitle;
	this->yWindowTitle = yWindowTitle;
	this->tWindowTitle = tWindowTitle;
}

std::string& Motion::getWindowTitleX()
{
	return this->xWindowTitle;
}
std::string& Motion::getWindowTitleY()
{
	return this->yWindowTitle;
}

std::string& Motion::getWindowTitleT()
{
	return this->tWindowTitle;
}