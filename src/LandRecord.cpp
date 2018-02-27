/*
 * LandRecord.cpp
 *
 *  Created on: Mar 15, 2015
 *      Author: Kyle
 */

#include "LandRecord.h"

#include <stdlib.h>
#include <string.h>

#include <math.h>

#include <string>
#include <sstream>

template < typename T > std::string to_string(T value)
{
	std::ostringstream os;
	os << value;
	return os.str();
}

int LandRecord::setUnknown()
{
	this->Unknown = 0x09;

	return 1;
}

int LandRecord::setCell(long CellX, long CellY)
{
	this->CellX = CellX;
	this->CellY = CellY;

	return 1;
}

int LandRecord::genFlatHeightMap(float offset)
{
	signed char temp[65][65];
	memset(temp, 0, 65*65*sizeof(char));

	return setHeightMap(temp, offset);
}

int LandRecord::genCornerCaseTest(float offset)
{
	signed char temp[65][65];
	memset(temp, 0, 65*65*sizeof(char));

	temp[0][0] = 20;
	temp[1][1] = 15;
	temp[0][1] = 15;
	temp[1][0] = 15;

	temp[64][0] = 20;
	temp[63][1] = 15;
	temp[63][0] = 15;
	temp[64][1] = 15;

	temp[64][64] = 20;
	temp[63][63] = 15;
	temp[64][63] = 15;
	temp[63][64] = 15;

	temp[0][64] = 20;
	temp[1][63] = 15;
	temp[0][63] = 15;
	temp[1][64] = 15;
	return setHeightMap(temp, offset);
}

int LandRecord::setHeightMap(signed char heightmap[65][65], float offset)
{
	this->Unknown1 = offset;
	this->Unknown2 = 0x00;
	this->Unknown3 = 0x0000;
	memcpy(this->HeightMap, heightmap, 65*65);
	return 1;
}

int LandRecord::printHeightMap(bool asciiHeightMapActive)
{
	printf("HeightMap (N vvv):\n");
	for (unsigned int i = 0; i < 65; i++)
	{
		for (unsigned int j = 0; j < 65; j++)
		{
			if (asciiHeightMapActive)
			{
				char value[2];
				asciiHeightToChar(this->HeightMap[i][j], value);
				printf("%s\t", value);
			}
			else
			{
				printf("%d\t", this->HeightMap[i][j]);
			}
		}
		printf("\n");
	}
	return 1;
}

void LandRecord::asciiHeightToChar(char num, char *buf)
{
	if (num < 5)
	{
		buf[0] = '.';
	}
	else if (num < 10)
	{
		buf[0] = ':';
	}
	else if (num < 15)
	{
		buf[0] = '&';
	}
	else
	{
		buf[0] = '#';
	}
	buf[1] = 0;
}

// Seems like for (i, j) j = 64 we have an differential offset vector that applies to a row at a time.
// SO height(i, j) = float offset + sum( height(n, 64), n = 0..i) ) + sum( height(i, n), n = 0..j) )
// So we have a 65x65 height map where row (i, 0) = float offset + sum( height(n, 64), n = 0..i )
//
// North is along increasing i, and so East is along increasing j.
int LandRecord::convertHeightMapToDiff()
{
	signed char temp[65][65];
	memset(temp, 0, 65 * 65 * sizeof(char));

	// point (0, 0) = float offset, soooo....
	this->Unknown1 = round(this->HeightMap[0][0]);

	// Now set the first column using the differential row offset vector...
	for (unsigned int i = 1; i < 65; i++)
	{
		temp[i-1][64] = this->HeightMap[i][0] - this->HeightMap[i-1][0];
	}

	// Now set all the other 64 columns using the differential row offset vector AND
	// each points differential summation vector.
	for (unsigned int i = 0; i < 65; i++)
	{
		for (unsigned int j = 0; j < 64; j++)
		{
			temp[i][j] = this->HeightMap[i][j+1] - this->HeightMap[i][j];
		}
	}

	memcpy(&(this->HeightMap), temp, 65*65);

	return 1;
}

int LandRecord::setNormalMap(normals normalmap)
{
	memcpy(this->NormalMap, normalmap, 65*65*3);
	return 1;
}

int LandRecord::setWorldMapPixels(std::string pixelArray)
{
	this->WorldMapPixels.clear();
	this->WorldMapPixels = pixelArray;

	return 1;
}

int LandRecord::setDataValues(ModSubRecord subRecord)
{
	if (strcmp(subRecord.name, "INTV") == 0)
	{
		printf("size: %d\n", subRecord.size);
		long *data = (long *) subRecord.data.data();
		this->CellX = data[0];
		this->CellY = data[1];
	}
	else if (strcmp(subRecord.name, "DATA") == 0)
	{
		long *data = (long *) subRecord.data.data();
		this->Unknown = data[0];
	}
	else if (strcmp(subRecord.name, "VNML") == 0)
	{
		normals *data = (normals *) subRecord.data.data();
		memcpy(this->NormalMap, data, 65*65*3);
	}
	else if (strcmp(subRecord.name, "VHGT") == 0)
	{
		float *data = (float *) subRecord.data.data();
		this->Unknown1 = data[0];
		char *data2 = (char *) &(data[1]);
		this->Unknown2 = data2[0];
		signed char *data3 = (signed char *) &(data2[1]);
		memcpy(this->HeightMap, data3, 65*65);
		short *data4 = (short *) &(data3[65*65]);
		this->Unknown3 = data4[0];
	}
	else if (strcmp(subRecord.name, "WNAM") == 0)
	{
		char *data = (char *) subRecord.data.data();
		this->WorldMapPixels.clear();
		this->WorldMapPixels.assign(data);
	}
	else
	{
		// Didn't recognize the subrecord type...
		return -1;
	}

	return 1;
}

int LandRecord::setRecordSize()
{
	long size = 0;

	// INTV - subrecord header + long CellX + long CellY
	size += 2 * sizeof(long) + 8;

	// DATA - subrecord header + long Unknown
	size += sizeof(long) + 8;

	// VNML - subrecord header + (65*65*3 bytes) normals
	size += sizeof(normals) + 8;

	// VHGT - subrecord header + float Unknown1 + char Unknown2 +
	//        (65*65 char) HeightMap + short Unknown3
	size += sizeof(float) + (65 * 65 + 1) * sizeof(char) + sizeof(short) + 8;

	// WNAM - subrecord header + (9*9 char) WorldMapPixels
	size += 81 * sizeof(char) + 8;

	this->size = size;

	return 1;
}

size_t LandRecord::exportToModFile(FILE *fid)
{
	size_t totalSize = 0;

	// LAND record header
	totalSize += fwrite("LAND", sizeof(char), 4, fid);
	totalSize += fwrite(&(this->size), sizeof(long), 1, fid);
	totalSize += fwrite(&(this->header1), sizeof(long), 1, fid);
	totalSize += fwrite(&(this->flags), sizeof(long), 1, fid);

	// INTV subrecord
	totalSize += fwrite("INTV", sizeof(char), 4, fid);
	long size = 2 * sizeof(long);
	totalSize += fwrite(&size, sizeof(long), 1, fid);
	totalSize += fwrite(&(this->CellX), sizeof(long), 1, fid);
	totalSize += fwrite(&(this->CellY), sizeof(long), 1, fid);

	// DATA subrecord
	totalSize += fwrite("DATA", sizeof(char), 4, fid);
	size = sizeof(long);
	totalSize += fwrite(&size, sizeof(long), 1, fid);
	totalSize += fwrite(&(this->Unknown), sizeof(long), 1, fid);

	// VNML subrecord
	totalSize += fwrite("VNML", sizeof(char), 4, fid);
	size = 1 * sizeof(normals);
	totalSize += fwrite(&size, sizeof(long), 1, fid);
	totalSize += fwrite(&(this->NormalMap), sizeof(normals), 1, fid);

	// VHGT subrecord
	totalSize += fwrite("VHGT", sizeof(char), 4, fid);
	size = (65 * 65 + 1) * sizeof(char) + sizeof(short) + sizeof(float);
	totalSize += fwrite(&size, sizeof(long), 1, fid);
	totalSize += fwrite(&(this->Unknown1), sizeof(float), 1, fid);
	totalSize += fwrite(&(this->Unknown2), sizeof(char), 1, fid);
	totalSize += fwrite(&(this->HeightMap), sizeof(char), 65 * 65, fid);
	totalSize += fwrite(&(this->Unknown3), sizeof(short), 1, fid);

	// WNAM subrecord
	totalSize += fwrite("WNAM", sizeof(char), 4, fid);
	size = 81 * sizeof(char);
	totalSize += fwrite(&size, sizeof(long), 1, fid);
	totalSize += fwrite(&(this->WorldMapPixels), sizeof(char), 81, fid);

	return totalSize;
}