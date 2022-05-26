#pragma once
#include <fstream>
#include <string>
#include <vector>
#include "shaderc/shaderc.h"
#include "h2bParser.h"

class LevelRenderer
{
public:

	//file io 
	std::fstream myFile;

	//Buffers
	VkDevice device = nullptr;
	VkBuffer indexHandle = nullptr;
	VkDeviceMemory indexData = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;

	//Matrices
	GW::MATH::GMATRIXF wMatrix;

	//H2BPARSER DATA MEMBERS
	H2B::VERTEX* meshVert;
	int* meshIndex;
	unsigned vertexCount;
	unsigned indexCount;
	unsigned materialCount;
	unsigned meshCount;
	std::vector<H2B::VERTEX> vertex;
	std::vector<unsigned> index;
	std::vector<H2B::MATERIAL> materials;
	std::vector<H2B::BATCH> batches;
	std::vector<H2B::MESH> meshes;

	LevelRenderer(std::string _objName, std::string _levelName)
	{
			//Looking for clones
			std::string h2bmeshes;
			int cloneID = _objName.find_first_of('.');
			h2bmeshes = _objName.substr(0, cloneID);

			//Get address name for model
			std::string odjAddress = "../../Assets/" + _levelName + "/Models/";
			std::string meshName = odjAddress.append(h2bmeshes);
			meshName = meshName.append(".h2b");

			//Pass to h2b
			H2B::Parser parserProxy;
			if (!parserProxy.Parse(meshName.c_str()))
			{
				std::cout << "File failed to open: " << meshName << std::endl;
			}

			//Assining the data
			vertexCount = parserProxy.vertexCount;
			indexCount = parserProxy.indexCount;
			materialCount = parserProxy.materialCount;
			meshCount = parserProxy.meshCount;
			vertex = parserProxy.vertices;
			index = parserProxy.indices;
			materials = parserProxy.materials;
			batches = parserProxy.batches;
			meshes = parserProxy.meshes;

			//Get material attrib
			for (unsigned i = 0; i < parserProxy.materialCount; i++)
			{
				materials[i].attrib = parserProxy.materials[i].attrib;
			}

		//Open gamelevel.txt
		std::string line;
		std::string gameLevelAddress = "../../Assets/" + _levelName + "/GameData/" + _levelName + ".txt";
		myFile.open(gameLevelAddress, std::ios::in);
		if (myFile.is_open())
		{
			//Read tho GameLevel.txt
			while (std::getline(myFile, line))
			{
				//Find first mesh
				if (line == "MESH" || line == "CAMERA")
				{
					//Read rest of the file
					while (std::getline(myFile, line))
					{
						//find the obj data
						if (line == _objName)
						{
							int counter = 0;
							//Read in all matrix data
							while (std::getline(myFile, line) && counter != 4)
							{
								counter++;
								std::vector<std::string> coordinates;
								std::string newline = line.substr(13);
								std::stringstream stream(newline);
								while (stream.good())
								{
									std::string numbers;
									std::getline(stream, numbers, ',');
									coordinates.push_back(numbers);
								}
								GW::MATH::GMATRIXF modelMatrix;
								switch (counter)
								{
								case 1:
									modelMatrix.row1.x = std::stof(coordinates[0]);
									modelMatrix.row1.y = std::stof(coordinates[1]);
									modelMatrix.row1.z = std::stof(coordinates[2]);
									modelMatrix.row1.w = std::stof(coordinates[3]);
									continue;
								case 2:
									modelMatrix.row2.x = std::stof(coordinates[0]);
									modelMatrix.row2.y = std::stof(coordinates[1]);
									modelMatrix.row2.z = std::stof(coordinates[2]);
									modelMatrix.row2.w = std::stof(coordinates[3]);
									continue;
								case 3:
									modelMatrix.row3.x = std::stof(coordinates[0]);
									modelMatrix.row3.y = std::stof(coordinates[1]);
									modelMatrix.row3.z = std::stof(coordinates[2]);
									modelMatrix.row3.w = std::stof(coordinates[3]);
									continue;
								case 4:
									modelMatrix.row4.x = std::stof(coordinates[0]);
									modelMatrix.row4.y = std::stof(coordinates[1]);
									modelMatrix.row4.z = std::stof(coordinates[2]);
									modelMatrix.row4.w = std::stof(coordinates[3]);
									wMatrix = modelMatrix;
									break;
								default:
									std::cout << "Error reading file: counter = " << counter << std::endl;
									continue;
								}
							}
						}
						else break;					}

				}
			}
			//Be sure to close file
			myFile.close();
		}
		else std::cout << "File failed to open: " << gameLevelAddress << std::endl;

		//Populate vertex
		meshVert = new H2B::VERTEX[vertexCount];
		for (unsigned i = 0; i < vertexCount; i++)
		{
			meshVert[i] = vertex[i];
		}
		//Populate index
		meshIndex = new int[indexCount];
		for (unsigned i = 0; i < indexCount; i++)
		{
			meshIndex[i] = index[i];
		}

	}

	//Get the camera cordinates to make positioning easier
	LevelRenderer(std::string _levelName)
	{
		//Open gamelevel.txt
		std::string line;
		std::string gameLevelAddress = "../../Assets/" + _levelName + "/GameData/" + _levelName + ".txt";
		myFile.open(gameLevelAddress, std::ios::in);
		if (myFile.is_open())
		{
			//Read tho GameLevel.txt
			while (std::getline(myFile, line))
			{
				//Find Camera
				if (line == "CAMERA")
				{
					//Read rest of the file
					while (std::getline(myFile, line))
					{
						//find the obj data
						if (line == "Camera")
						{
							int counter = 0;
							//Read in all matrix data
							while (std::getline(myFile, line) && counter != 4)
							{
								counter++;
								std::vector<std::string> coordinates;
								std::string newline = line.substr(13);
								std::stringstream stream(newline);
								while (stream.good())
								{
									std::string numbers;
									std::getline(stream, numbers, ',');
									coordinates.push_back(numbers);
								}
								GW::MATH::GMATRIXF modelMatrix;
								switch (counter)
								{
								case 1:
									modelMatrix.row1.x = std::stof(coordinates[0]);
									modelMatrix.row1.y = std::stof(coordinates[1]);
									modelMatrix.row1.z = std::stof(coordinates[2]);
									modelMatrix.row1.w = std::stof(coordinates[3]);
									continue;
								case 2:
									modelMatrix.row2.x = std::stof(coordinates[0]);
									modelMatrix.row2.y = std::stof(coordinates[1]);
									modelMatrix.row2.z = std::stof(coordinates[2]);
									modelMatrix.row2.w = std::stof(coordinates[3]);
									continue;
								case 3:
									modelMatrix.row3.x = std::stof(coordinates[0]);
									modelMatrix.row3.y = std::stof(coordinates[1]);
									modelMatrix.row3.z = std::stof(coordinates[2]);
									modelMatrix.row3.w = std::stof(coordinates[3]);
									continue;
								case 4:
									modelMatrix.row4.x = std::stof(coordinates[0]);
									modelMatrix.row4.y = std::stof(coordinates[1]);
									modelMatrix.row4.z = std::stof(coordinates[2]);
									modelMatrix.row4.w = std::stof(coordinates[3]);
									wMatrix = modelMatrix;
									break;
								default:
									std::cout << "Error reading file: counter = " << counter << std::endl;
									continue;
								}
							}
						}
						else break;
					}

				}
			}
			//Be sure to close file
			myFile.close();
		}
		else std::cout << "File failed to open: " << gameLevelAddress << std::endl;
	}
};