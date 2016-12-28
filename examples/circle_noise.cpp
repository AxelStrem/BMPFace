// Generates circular noise samples
//

#include "stdafx.h"
#include "BMPFace.hpp"

#include <string>
#include <sstream>
#include <iostream>

struct ClosestPoint
{
	int x;
	int y;
	float dist;
};

float distance(int x1,int y1,int x2, int y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	float r = dx*dx + dy*dy;
	return sqrt(r);
}

void InvertCircle(Bitmap<unsigned char> &bmp, int xc, int yc, float r)
{
	for (auto& p : bmp)
	{
		if (distance(xc, yc, p.x, p.y) <= r)
		{
			p.value = 1 - p.value;
		}
	}
}

void InvertSemiplane(Bitmap<unsigned char> &bmp, int xc, int yc, float vx, float vy)
{
	for (auto& p : bmp)
	{
		int dx = p.x - xc;
		int dy = p.y - yc;
		float scalar = vx*dx + vy*dy;
		if (scalar >= 0.f)
		{
			p.value = 1 - p.value;
		}
	}
}


void InvertFill(Bitmap<unsigned char> &bmp, int xc, int yc)
{
	bmp.Fill(xc, yc, [](Bitmap<unsigned char>::BitmapPoint &bp)->void 
		{ 
			bp.value = 1 - bp.value; 
		});
}

int main()
{
	Bitmap<unsigned char> bmp;

	bmp.Palette(0) = RGB(0, 0, 0);
	for (int i = 1; i < 256; i++)
	{
		bmp.Palette(i) = 0xFFFFFF;
	}

	bmp.SetSize(600, 600);

	for (int i = 110; i < 2048; i++)
	{
		bmp.Clear(0);
		int iter = (rand() % 3 + 1);
		for (int j = 0; j < iter; j++)
		{
			int am = rand() % 4096;

			float feature_size = 1.f / am;

			while (am-- > 0)
			{
				float r = (rand() % (20 * max(bmp.GetHeight(), bmp.GetWidth())));
				r *= feature_size;
				int xc = rand() % (bmp.GetWidth() + static_cast<int>(2 * r)) - static_cast<int>(r);
				int yc = rand() % (bmp.GetHeight() + static_cast<int>(2 * r)) - static_cast<int>(r);

				float vx = (rand() % 512 - 256) / 256.f;
				float vy = (rand() % 512 - 256) / 256.f;

				float feature = (rand() % 1024) / 1024.f;
				if (feature > feature_size)
					InvertCircle(bmp, xc, yc, r);
				else
					InvertSemiplane(bmp, xc, yc, vx, vy);

				if ((rand() % 4096) < 3)
				{
					InvertFill(bmp, rand() % bmp.GetWidth(), rand() % bmp.GetHeight());
					std::cout << "FILL" << std::endl;
				}
			}
		}

    std::string name;
		cin>>name;
		bmp.Save(name);
	}
}
