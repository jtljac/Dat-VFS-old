#pragma once
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "DataPtr.h"

struct Path {
	std::vector<std::string> path;

	Path() {}

	Path(std::filesystem::path Path) {

		for (const std::filesystem::path& part : Path) {
			if (part.string() != "/" && part.string() != "\\") path.push_back(part.string());
		}

		//int index;
		//// while theres still some string left
		//while (path.size()) {
		//	// find the first slash
		//	index = path.find_first_of("\\/");
		//	// if the slash is at the begining of the string, discard it
		//	if (index == 0) path = path.substr(index + 1);

		//	else if (index != std::string::npos) {
		//		path.push_back(path.substr(0, index));
		//		path = path.substr(index + 1);
		//	}

		//	else {
		//		path.push_back(path);
		//		path = "";
		//	}
		//}
	}

	Path(std::vector<std::string> Path) {
		path = Path;
	}

	/**
	 * Gets the folder name at the given index
	 * @param Item The index of the path to get
	 * @return The folder name at the given index
	 */
	std::string operator[](unsigned int Item) {
		if (Item >= path.size()) {
			throw std::out_of_range("Index out of range");
		}

		return path[Item];
	}

	/**
	 * Gets the sub path from starting from the given start and ending at the given end
	 * @param Start The index to start the subpath with
	 * @param End The index to end the subpath with
	 * @return the resulting subpath
	 */
	Path getSubPath(unsigned int Start, unsigned int End) {
		if (End >= path.size() || Start >= path.size()) throw std::out_of_range("Index out of range");
		return Path(std::vector<std::string>(path.begin() + Start, path.begin() + End));
	}

	/**
	 * Gets the sub path from starting from the given start to the end of the path
	 * @param start The index to start the subpath with
	 * @return the resulting subpath
	 */
	Path getSubPath(unsigned int start) {
		if (start >= path.size()) throw std::out_of_range("Index out of range");
		return Path(std::vector<std::string>(path.begin() + start, path.end()));
	}

	friend Path operator+(Path& thisPath, Path& otherPath) {
		std::vector<std::string> newPath(thisPath.path);
		newPath.reserve(thisPath.path.size() + otherPath.path.size());
		newPath.insert(newPath.end(), otherPath.path.begin(), otherPath.path.end());
		return Path(newPath);
	}

	/**
	 * Adds the given path onto the end
	 * @param OtherPath
	 */
	Path& append(Path& OtherPath) {
		path.reserve(path.size() + OtherPath.path.size());
		path.insert(path.end(), OtherPath.path.begin(), OtherPath.path.end());
		return *this;
	}

	explicit operator std::string() {
		std::string theString = "";
		for (std::string item : path) {
			theString.append("/" + item);
		}
		return theString;
	}

	/**
	 * Gets how many folders deep the path goes
	 */
	int totalDepth() {
		return path.size() - 1;
	}

	/**
	 * Gets the Last item in the path
	 */
	std::string lastItem() {
		return path[totalDepth()];
	}
};

class DVFSFile {
protected:
	DataPtr data = DataPtr(nullptr, 1);
	uint32_t dataSize;
	std::filesystem::path fileLocation;

	/**
	 * Loads the data into the memory if it isn't already
	 * @return Whether the file was loaded into memory or not
	 */
	bool loadFile() {
		// Ensure its not already loaded
		if (data.dataLoaded()) {
			std::cout << "Tried to load file while already loaded, ignoring" << std::endl;
			return false;
		}

		// Get the file and check we successfully got it
		std::ifstream theFile(fileLocation, std::ios::in | std::ios::binary);
		if (!theFile) {
			std::cout << "Unable to load file" << std::endl;
			return false;
		}

		// Find the size of the file, calculate the size, then return to the beginning
		theFile.seekg(0, std::ios::end);
		dataSize = theFile.tellg();
		theFile.seekg(0);

		// Create a big enough buffer
		data.setData(new char[dataSize]);

		// Read the data in
		theFile.read(data.get(), dataSize);

		data.setLoaded(true);

		theFile.close();
		return true;
	}



public:
	DVFSFile(std::filesystem::path Path) {
		fileLocation = Path;
	}

	~DVFSFile() {
		data.cleanup();
	}

	DataPtr getFile() {
		if (!data.dataLoaded()) {
			loadFile();
		}

		return data;
	}

	std::filesystem::path getDest() {
		return fileLocation;
	}
};

class VFSArchive {
public:
	/**
	 * Gets a vector containing all files in the archive as a DVFSFile entry, and its path
	 */
	virtual std::vector<std::pair<Path, DVFSFile*>> getFiles() = 0;
};