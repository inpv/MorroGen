/*
 * CellRecord.cpp
 *
 *  Created on: Mar 15, 2015
 *      Author: Kyle
 */

#include "CellRecord.h"

#include <stdio.h>
#include <string.h>

#include <cstdint>
#include <string>
#include <sstream>

template < typename T > std::string to_string(T value)
{
  std::ostringstream os;
  os << value;
  return os.str();
}

int CellRecord::setIdString(std::string name)
{
  this->IdString.clear();
  name.push_back(0);
  this->IdString = name;

  return 1;
}

int CellRecord::setRegionName(std::string name)
{
  this->RegionName.clear();
  name.push_back(0);
  this->RegionName = name;

  return 1;
}

int CellRecord::setGridAndFlags(int GridX, int GridY, int Flags)
{
  this->GridX = GridX;
  this->GridY = GridY;
  this->Flags = Flags;

  return 1;
}

int CellRecord::setRecordSize()
{
  std::uint32_t size = 0;

  // NAME subrecord header and ID string
  size += this->IdString.size() + 8;

  // DATA - uint32_t Flags + uint32_t GridX + uint32_t GridY + subrecord header
  size += 3 * sizeof(std::uint32_t) + 8;

  // RGNN - string size + subrecord header
  if (this->RegionName.size() > 0)
  {
    // If it exists, write it out
    size += this->RegionName.size() + 8;
  }

  if (this->objectReferences.size() > 0)
  {
    // NAM0 - # objects in objectReferences
    size += sizeof(std::uint32_t) + 8;

    for (std::uint32_t i = 0; i < this->objectReferences.size(); i++)
    {
      // FRMR - object index in objectReferences
      size += sizeof(std::uint32_t) + 8;

      // NAME - object name
      size += this->objectReferences[i].name.size() + 8;

      // DATA - object positional and rotational data
      size += 6 * sizeof(float) + 8;
    }
  }

  this->size = size;

  return 1;
}

int CellRecord::setDataValues(ModSubRecord subRecord)
{
  if (strcmp(subRecord.name, "NAME") == 0)
  {
    printf("size: %d\n", subRecord.size);
    char *data = (char *) subRecord.data.data();
    this->IdString.clear();
    for (int i = 0; i < subRecord.size; i++)
    {
      this->IdString[i] = data[i];
    }
  }
  else if (strcmp(subRecord.name, "DATA") == 0)
  {
    std::uint32_t *data = (std::uint32_t *) subRecord.data.data();
    this->Flags = data[0];
    this->GridX = data[1];
    this->GridY = data[2];
  }
  else if (strcmp(subRecord.name, "RGNN") == 0)
  {
    char *data = (char *) subRecord.data.data();
    this->RegionName.clear();
    for (int i = 0; i < subRecord.size; i++)
    {
      this->RegionName[i] = data[i];
    }
  }
  else
  {
    // Didn't recognize the subrecord type...
    return -1;
  }

  return 1;
}

void CellRecord::addObjectToCell(std::string name, PosRotData data)
{
  ObjectReference objref;
  objref.name = name;
  objref.data = data;
  this->objectReferences.push_back(objref);
}

std::string CellRecord::exportToModData()
{
  std::string outputFile;

  // Write CELL record header
  // char Name[4]
  // uint32_t Size
  // uint32_t Header1
  // uint32_t Flags - 0x0000200 = Blocked, 0x00000400 = Persistant
  // followed by subrecords
  std::string data("CELL");
  outputFile += to_string(this->size);
  outputFile += to_string(this->header1);
  outputFile += to_string(this->flags);

  // Each subrecord has a header of
  // char Name[4]
  // uint32_t Size
  // followed by data.

  // Write NAME subrecord
  // Id String
  outputFile += "NAME";
  outputFile += to_string(this->IdString.size());

  // DATA subrecord
  outputFile += "DATA";
  outputFile += to_string(this->Flags);
  outputFile += to_string(this->GridX);
  outputFile += to_string(this->GridY);

  // RGNN subrecord
  if (this->RegionName.size() > 0)
  {
    // If it exists, write it out
    outputFile += this->RegionName;
  }

  // Object references
  if (this->objectReferences.size() > 0)
  {
    // NAM0 subrecord
    outputFile += "NAM0";
    outputFile += to_string(this->objectReferences.size());

    for (std::uint32_t i = 0; i < this->objectReferences.size(); i++)
    {
      // FRMR subrecord
      outputFile += "FRMR";
      outputFile += to_string(i + 1);

      // NAME subrecord
      outputFile += "NAME";
      outputFile += this->objectReferences[i].name;

      // DATA subrecord
      outputFile += "DATA";
      outputFile += to_string(this->objectReferences[i].data.posX);
      outputFile += to_string(this->objectReferences[i].data.posY);
      outputFile += to_string(this->objectReferences[i].data.posZ);
      outputFile += to_string(this->objectReferences[i].data.rotX);
      outputFile += to_string(this->objectReferences[i].data.rotY);
      outputFile += to_string(this->objectReferences[i].data.rotZ);
    }
  }

  return outputFile;
}

size_t CellRecord::exportToModFile(FILE *fid)
{
  size_t totalSize = 0;

  // CELL record header
  totalSize += fwrite("CELL", sizeof(char), 4, fid);
  totalSize += fwrite(&(this->size), sizeof(std::uint32_t), 1, fid);
  totalSize += fwrite(&(this->header1), sizeof(std::uint32_t), 1, fid);
  totalSize += fwrite(&(this->flags), sizeof(std::uint32_t), 1, fid);

  // NAME subrecord
  totalSize += fwrite("NAME", sizeof(char), 4, fid);
  std::uint32_t size = this->IdString.size();
  totalSize += fwrite(&size, sizeof(std::uint32_t), 1, fid);
  totalSize += fwrite(this->IdString.c_str(), sizeof(char), this->IdString.size(), fid);

  // DATA subrecord
  totalSize += fwrite("DATA", sizeof(char), 4, fid);
  size = 3 * sizeof(std::uint32_t);
  totalSize += fwrite(&size, sizeof(std::uint32_t), 1, fid);
  totalSize += fwrite(&(this->Flags), sizeof(std::uint32_t), 1, fid);
  totalSize += fwrite(&(this->GridX), sizeof(std::uint32_t), 1, fid);
  totalSize += fwrite(&(this->GridY), sizeof(std::uint32_t), 1, fid);

  // RGNN subrecord
  if (this->RegionName.size() > 0)
  {
    // If it exists, write it out
    totalSize += fwrite("RGNN", sizeof(char), 4, fid);
    size = this->RegionName.size() * sizeof(char);
    totalSize += fwrite(&size, sizeof(std::uint32_t), 1, fid);
    totalSize += fwrite(this->RegionName.c_str(), sizeof(char), this->RegionName.size(), fid);
  }

  // Object references
  if (this->objectReferences.size() > 0)
  {
    // NAM0 subrecord
    totalSize += fwrite("NAM0", sizeof(char), 4, fid);
    size = sizeof(std::uint32_t);
    totalSize += fwrite(&size, sizeof(std::uint32_t), 1, fid);
    std::uint32_t nObjects = this->objectReferences.size();
    totalSize += fwrite(&nObjects, sizeof(std::uint32_t), 1, fid);

    for (std::uint32_t i = 0; i < this->objectReferences.size(); i++)
    {
      // FRMR subrecord
      totalSize += fwrite("FRMR", sizeof(char), 4, fid);
      size = sizeof(std::uint32_t);
      totalSize += fwrite(&size, sizeof(std::uint32_t), 1, fid);
      std::uint32_t idx = i + 1;
      totalSize += fwrite(&idx, sizeof(std::uint32_t), 1, fid);

      // NAME subrecord
      totalSize += fwrite("NAME", sizeof(char), 4, fid);
      size = this->objectReferences[i].name.size() * sizeof(char);
      totalSize += fwrite(&size, sizeof(std::uint32_t), 1, fid);
      totalSize += fwrite(this->objectReferences[i].name.c_str(), sizeof(char),
                          this->objectReferences[i].name.size(), fid);

      // DATA subrecord
      totalSize += fwrite("DATA", sizeof(char), 4, fid);
      size = 6 * sizeof(float);
      totalSize += fwrite(&size, sizeof(std::uint32_t), 1, fid);
      totalSize += fwrite(&(this->objectReferences[i].data.posX),
                          sizeof(float), 1, fid);
      totalSize += fwrite(&(this->objectReferences[i].data.posY),
                          sizeof(float), 1, fid);
      totalSize += fwrite(&(this->objectReferences[i].data.posZ),
                          sizeof(float), 1, fid);
      totalSize += fwrite(&(this->objectReferences[i].data.rotX),
                          sizeof(float), 1, fid);
      totalSize += fwrite(&(this->objectReferences[i].data.rotY),
                          sizeof(float), 1, fid);
      totalSize += fwrite(&(this->objectReferences[i].data.rotZ),
                          sizeof(float), 1, fid);

    }
  }

  return totalSize;
}
