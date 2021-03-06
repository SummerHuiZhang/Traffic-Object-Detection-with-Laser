#include "find_edge.v2.h"

#define Z -30
#define dX -50
#define dY 50
#define F1 7.1695537050642884e+02
#define F2 7.1275153332443017e+02
#define D1 3.3311050187491378e+02
#define D2 2.8336132788893195e+02
#define LENGTH 100
#define RANGE 10


void FindEdge::mb_init(sensor_msgs::LaserScan& scan)
{
	vector<float> data = scan.ranges;
	float radian = scan.angle_min;
	float angle_increment = scan.angle_increment;

	m_size = data.size();
	depth = Mat::zeros(m_size, 1, CV_32FC1);
	m_coord = Mat::zeros(m_size, 2, CV_32FC1);
	for (int i = 0; i < m_size; i++)
	{
		float l = data[i]*1000;
		depth.at<float>(i, 0) = l;
		float x = static_cast<float>(l * cos(radian));
		float y = static_cast<float>(l * sin(radian));
		m_coord.at<float>(i, 0) = x;
		m_coord.at<float>(i, 1) = y;
		radian += angle_increment;
	}
}

void FindEdge::mb_mapping(IplImage *&img)
{
	CvPoint point;
	int X = 0;
	int max = -3000, min = 3000;
	int width = img->width;
	int height = img->height;
	float x,y;
	m_plain = Mat::zeros(m_size, 2, CV_32FC1);
	for (int i = 0; i < m_size; i++)
	{
		m_coord.at<float>(i,0) += dX;
		X = m_coord.at<float>(i, 0);
		x = m_coord.at<float>(i, 1) * F1 / X + D1;
		y = Z * F2 / X + D2;
	
		x = width - x;
		y = height - y;
		x += dY;
        if(x < 0 || x > width || y < 0 || y > height)
  		    continue;
		
		m_plain.at<float>(i,0) = x;
		m_plain.at<float>(i,1) = y;

		/*if (point.y > max)
			max = point.y;
		else if (point.y < min)
			min = point.y;*/
		cvDrawLine(img, point, point, cvScalar(255, 0, 0), 5);
	}
	m_min = min;
	m_max = max;
}

void FindEdge::mb_cluster()
{
	int range[RANGE] = {0};
	int average[RANGE] = {0};
	for (int i = 0; i < m_size; i++)
	{
		int t = m_plain.at<float>(i,1);
		int x = t / 50;
		if (x < RANGE && x >= 0)
		{
			range[x]++;
			average[x] += t;
		}
	}
	for (int i = 0; i < RANGE; i++)
	{
		if (range[i] != 0)
			average[i] /= range[i];
		if (average[i] > 20)
		{
			m_standard = average[i];
			m_standard += 10;
			cout << m_standard;
			break;
		}
	}
	cout << endl;
}

void FindEdge::mb_findEdge()
{
	pre_object = m_object;
	m_object.clear();
	m_mark.clear();

	int average = 0;
	int num = 0;
	for (int i = 0; i < m_size; i++)
	{
		int x = m_plain.at<float>(i,1);
		if (x > 0 && x < 500)
		{
			num++;
			average += x;
		}
	}
	average /= num;
	average += 10;

	for (int i = 0; i < m_size; i++)
	{
		if (m_plain.at<float>(i,1)> m_standard)
			m_mark.push_back(1);
		else
			m_mark.push_back(0);
	}

	for (int i = 0; i < m_size; i++)
	{
		int count = 0;
		for (int j = -5; j < 5; j++)
		{
			int x = i + j;
			if (x < m_size && x >= 0 && m_mark[x] == 1)
				count++;
		}
		if (count > 6)
			m_mark[i] = 1;
		else if (count < 4)
			m_mark[i] = 0;
	}

	m_Obj object;
	for (int i = 0; i < m_size; i++)
	{
		if (m_mark[i] == 1)
		{
			object.right_index = i;
			while (i < m_size && m_mark[i] == 1)
				i++;
			object.left_index = i-1;
			m_object.push_back(object);
		}
	}
}

void FindEdge::mb_drawRec(IplImage *&img)
{
	pre_rect = m_rect;
	m_rect.clear();
	CvRect rect;
	rect.height = LENGTH;
	int size = m_object.size();
	if (pre_object.size() != 0 && pre_rect.size() != 0)
	{
		for (int i = 0, j = 0; i < size; i++)
		{
			for (; j < pre_object.size(); j++)
				if (abs(pre_object[j].left_index - m_object[i].left_index) + abs(pre_object[j].right_index - m_object[i].right_index) <= 3)
				{
					rect = pre_rect[j];
					break;
				}
			if (j == pre_object.size())
			{
				rect.x = m_plain.at<float>(m_object[i].left_index,0);
				rect.y = m_plain.at<float>(m_object[i].left_index,1) - LENGTH;
				rect.width = abs(m_plain.at<float>(m_object[i].right_index,0) - m_plain.at<float>(m_object[i].left_index,0));
			}
			m_rect.push_back(rect);
			cvRectangleR(img, rect, cvScalar(255, 0, 0), 5);
		}
	}
	else
	{
		for (int i = 0; i < size; i++)
		{
			rect.x = m_plain.at<float>(m_object[i].left_index,0);
			rect.y = m_plain.at<float>(m_object[i].left_index,1) - LENGTH;
			rect.width = abs(m_plain.at<float>(m_object[i].right_index,0) - m_plain.at<float>(m_object[i].left_index,0));
			m_rect.push_back(rect);
			cvRectangleR(img, rect, cvScalar(255, 0, 0), 5);
		}
	}
}

void FindEdge::mb_run(sensor_msgs::LaserScan& scan, IplImage *&img)
{
	mb_init(scan);
	mb_mapping(img);
	mb_cluster();
	mb_findEdge();
	mb_drawRec(img);
}
