#pragma once
#include <windows.h>

#include "stdafx.h"
#include <iterator>

#include <vector> 
#include <string>
#include <memory>

#include <fstream>
#include <array>

#include <windows.h>
#include <conio.h>

COLORREF BGR(COLORREF c)
{
	return RGB(GetBValue(c), GetGValue(c), GetRValue(c));
}

struct COLOR_3B
{
	BYTE b;
	BYTE g;
	BYTE r;
};

struct COLOR_4B : public COLOR_3B
{
	BYTE a;
};

bool in_palette(COLOR_4B col)
{
	return (col.a | col.b | col.g | col.r) != 0;
}

template<class Pixel, class Color = COLOR_3B> class Bitmap
{
	int size_x, size_y;
	std::vector<Pixel> data;
	std::vector<COLORREF> palette;

	BITMAPFILEHEADER hdr;
	BITMAPINFOHEADER ihdr;
	BITMAPCOREHEADER chdr;

public:
	int GetWidth() { return size_x; }
	int GetHeight() { return size_y; }

	void SetSize(int width, int height)
	{
		size_x = width;
		size_y = height;
		data.resize(size_x*size_y);
	}

	Pixel& At(int x, int y)
	{
		return data[size_x*y + x];
	}

	bool Check(int x, int y)
	{
		return ((x < size_x) && (x >= 0) && (y < size_y) && (y >= 0));
	}

	COLORREF Palette(Pixel value) const
	{
		return palette[value];
	}

	bool Load(std::string filename)
	{
		std::ifstream src;
		src.open(filename.c_str(), std::ios_base::binary);
		src.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
		src.read(reinterpret_cast<char*>(&ihdr), sizeof(ihdr));

		if (ihdr.biBitCount != (8 * sizeof(Pixel))) 
			return false;

		int offset = sizeof(hdr) + sizeof(ihdr);

		if (sizeof(Pixel) == 1)
		{
			palette.resize(256);
			for (COLORREF& c : palette)
			{
				src.read(reinterpret_cast<char*>(&c), sizeof(COLORREF));
				c = BGR(c);
			}
			offset += 256 * sizeof(COLORREF);
		}

		unsigned char ch;

		int height = ihdr.biHeight;
		int width = ihdr.biWidth;
		int offbits = hdr.bfOffBits;

		int stride = ((width*sizeof(Pixel) + 3) / 4) * 4;

		while (offset < hdr.bfOffBits)
		{
			offset++;
			src.read(reinterpret_cast<char*>(&ch), 1);
		}

		SetSize(width, height);
		Pixel p;

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				src.read(reinterpret_cast<char*>(&p), sizeof(Pixel));
				At(j, height-1-i) = p;
			}
			offset = width*sizeof(Pixel);
			while (offset < stride)
			{
				offset++;
				src.read(reinterpret_cast<char*>(&ch), 1);
			}
		}

		src.close();
		return true;
	}

	void Save(std::string filename)
	{
		ZeroMemory(&hdr, sizeof(hdr));
		ZeroMemory(&ihdr, sizeof(ihdr));
		
		ihdr.biBitCount = 8 * sizeof(Pixel);

		ihdr.biHeight = size_y;
		ihdr.biWidth = size_x;
		hdr.bfOffBits = sizeof(hdr) + sizeof(ihdr) + (sizeof(Pixel)==1)?(256*sizeof(COLORREF)):0;

		int height = ihdr.biHeight;
		int width = ihdr.biWidth;
		int offbits = hdr.bfOffBits;

		int stride = ((width * sizeof(Pixel) + 3) / 4) * 4;
		int datasize = stride*height;

		hdr.bfType = ('M' << 8) | 'B';
		hdr.bfSize = hdr.bfOffBits+datasize;

		ihdr.biSize = sizeof(ihdr);
		ihdr.biPlanes = 1;
		ihdr.biCompression = BI_RGB;
		
		std::ofstream dst;
		dst.open(filename.c_str(), std::ios_base::binary);
		dst.write(reinterpret_cast<char*>(&hdr), sizeof(hdr));
		dst.write(reinterpret_cast<char*>(&ihdr), sizeof(ihdr));
				
		int offset = sizeof(hdr) + sizeof(ihdr);

		if (sizeof(Pixel) == 1)
		{
			palette.resize(256);
			for (COLORREF& c : palette)
			{
				COLORREF cc = BGR(c);
				dst.write(reinterpret_cast<char*>(&cc), sizeof(COLORREF));
			}
			offset += 256 * sizeof(COLORREF);
		}

		unsigned char ch = 0;

		while (offset < hdr.bfOffBits)
		{
			offset++;
			dst.write(reinterpret_cast<char*>(&ch), 1);
		}

		Pixel p;

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				p = At(j, height - 1 - i);
				dst.write(reinterpret_cast<char*>(&p), sizeof(Pixel));
			}
			offset = width * sizeof(Pixel);
			while (offset < stride)
			{
				offset++;
				dst.write(reinterpret_cast<char*>(&ch), 1);
			}
		}

		dst.close();
	}

	struct BitmapPoint
	{
		int x;
		int y;
		Pixel &value;
	};

	class Iterator
	{
		Bitmap& host;
		int x;
		int y;
	public:
		Iterator(Bitmap& h) : host(h) {}
		Iterator(Bitmap& h, int x_, int y_) : host(h), x(x_), y(y_) {}
		Iterator operator ++()
		{
			x++;
			if (x >= host.GetWidth())
			{
				x = 0;
				y++;
			}
			return *this;
		}

		bool operator==(const Iterator& b) const
		{
			return (x == b.x) && (y == b.y);
	    }

		bool operator!=(const Iterator& b) const
		{
			return !(*this==b);
		}

		BitmapPoint operator*()
		{
			return BitmapPoint{ x,y,host.At(x,y) };
		}
	};

	Iterator begin()
	{
		return Iterator(*this,0,0);
	}

	Iterator end()
	{
		return Iterator(*this, 0, size_y);
	}

	std::vector<BitmapPoint> Aperture(int x, int y)
	{
		std::vector<BitmapPoint> result;
		for(int i=-1;i<=1;i++)
			for (int j = -1; j <= 1; j++)
			{
				if ((i == 0) && (j==0)) continue;
				if (!Check(x + i, y + j)) continue;
				result.push_back(BitmapPoint{x+i,y+j,At(x+i,y+j)});
			}

		return result;
	}
};
