#pragma once
#include <stdint.h>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <assert.h>
#include <regex>
#include "DatVFS/DatVFSCommon.h"

class DatVFS {
	std::unordered_map<std::string, DatVFS*> folders;
	std::unordered_map<std::string, DVFSFile*> files;

	/**
	 * Adds the given file to the VFS
	 * @param FilePath The path to the file
	 * @param DestFileName The name that the file will have in the VFS
	 * @param Overwrite Whether to Overwrite the destination file if it already exists
	 * @param Regex The Regex string the filename must match to be added to the filesystem
	 */
	void addFile(const std::filesystem::path& FilePath, const std::string DestFileName, bool Overwrite = false, const std::string Regex = "(.*)") {
		// Find the file name
		std::string ActualFileName = FilePath.filename().string();

		// Check it matches the regex
		if (std::regex_match(ActualFileName.c_str(), std::regex(Regex))) {
			addFile(DestFileName, new DVFSFile(FilePath), Overwrite);
		}
	}

	/**
	 * Adds the given file to the VFS
	 * @param DestFileName The name of the file in the VFS
	 * @param File A Pointer to a setup DVFSFile
	 * @param Overwrite Whether to Overwrite the destination file if it already exists
	 */
	void addFile(const std::string DestFileName, DVFSFile* File, bool Overwrite = false) {
		// Check the file doesn't already exist
		if (files.count(DestFileName)) {
			std::cout << "File already exists";
			if (!Overwrite) {
				std::cout << std::endl;
				return;
			}
			else {
				std::cout << ", overwriting" << std::endl;
				delete files[DestFileName];
			}
		}
		files[DestFileName] = File;
	}

	/**
	 * Gets the file at the given location
	 * @param thePath The path to the file
	 * @return The file at the given location
	 */
	DVFSFile* getFileInternal(Path thePath) {
		if (thePath.totalDepth() > 0) {
			if (folders.count(thePath[0])) {
				return folders[thePath[0]]->getFileInternal(thePath.getSubPath(1));
			}
		}
		else {
			if (files.count(thePath[0])) {
				return files[thePath[0]];
			}
		}

		return nullptr;
	}

	DatVFS* getFolderInternal(Path thePath) {
		if (thePath.totalDepth() > 0) {
			if (folders.count(thePath[0])) {
				return folders[thePath[0]]->getFolderInternal(thePath.getSubPath(1));
			}
		}
		else {
			if (folders.count(thePath[0])) {
				return folders[thePath[0]];
			}
		}

		return nullptr;
	}

	void getAllFilesThatMatchRegexInternal(const std::string Regex, std::vector<DVFSFile*>& Files) {
		// Count files that match
		for (std::unordered_map<std::string, DVFSFile*>::iterator it = files.begin(); it != files.end();) {
			if (std::regex_match(it->first, std::regex(Regex))) {
				Files.push_back(it->second);
			}
		}

		// add count of each subdirectories
		for (std::unordered_map<std::string, DatVFS*>::iterator it = folders.begin(); it != folders.end();) {
			it->second->getAllFilesThatMatchRegexInternal(Regex, Files);
		}
	}

public:
	~DatVFS() {
		// Clear all the sub folders
		for (std::unordered_map<std::string, DatVFS*>::iterator it = folders.begin(); it != folders.end(); ++it) {
			delete it->second;
		}

		// Remove sub folders
		folders.clear();

		// Clear all the files
		for (std::unordered_map<std::string, DVFSFile*>::iterator it = files.begin(); it != files.end(); ++it) {
			delete it->second;
		}

		files.clear();
	}

	/**
	 * Adds the given file to the VFS
	 * @param FilePath the path to the file
	 * @param VirtualPath The path in the VFS (Including the destination filename) from this point to put the file
	 * @param Overwrite Whether to Overwrite the destination file if it already exists
	 * @param Regex The Regex string the filename must match to be added to the filesystem
	 */
	void addFile(const std::filesystem::path& FilePath, Path VirtualPath, bool Overwrite = false, const std::string Regex = "(.*)") {
		// Check to see if we're in the write directory of the VFS, otherwise pass to the next level
		if (VirtualPath.totalDepth() > 0) {
			if (!folders.count(VirtualPath[0])) {
				folders.emplace(VirtualPath[0], new DatVFS());
			}
			folders[VirtualPath[0]]->addFile(FilePath, VirtualPath.getSubPath(1), Overwrite, Regex);
		}
		else {
			addFile(FilePath, VirtualPath[0], Overwrite, Regex);
		}
	}

	/**
	 * Adds the given file to the VFS
	 * @param File A Pointer to a setup DVFSFile
	 * @param VirtualPath The path in the VFS (Including the destination filename) from this point to put the file
	 * @param Overwrite Whether to Overwrite the destination file if it already exists
	 */
	void addFile(DVFSFile* File, Path virtualPath, bool overwrite = false) {
		// Check to see if we're in the write directory of the VFS, otherwise pass to the next level
		if (virtualPath.totalDepth() > 0) {
			if (!folders.count(virtualPath[0])) {
				folders.emplace(virtualPath[0], new DatVFS());
			}
			folders[virtualPath[0]]->addFile(File, virtualPath.getSubPath(1), overwrite);
		}
		else {
			addFile(virtualPath[0], File, overwrite);
		}
	}

	/**
	 * Adds all the files in the given Folder to the VFS
	 * @param FilePath the path to the Folder
	 * @param Overwrite Whether to Overwrite the destination file if it already exists
	 * @param Regex The Regex string each filename must match to be added to the filesystem
	 */
	void addFolder(const std::filesystem::path& Folder, bool Overwrite = false, bool Recursive = true, const std::string Regex = "(.*)") {
		std::filesystem::directory_iterator directories(Folder);

		// Iterate through all files and folders in this directory
		for (const std::filesystem::directory_entry& entry : directories) {
			Path thePath(entry.path());

			// If it's a directory, enter and add all the sub files
			if (entry.is_directory() && Recursive) {
				if (!folders.count(thePath.lastItem())) {
					folders.emplace(thePath.lastItem(), new DatVFS());
				}
				folders[thePath.lastItem()]->addFolder(entry.path(), Overwrite, Recursive, Regex);
			}
			else {
				addFile(entry.path(), thePath.lastItem(), Overwrite, Regex);
			}
		}
	}

	/**
	 * Adds all the files in the given Folder to the VFS
	 * @param FilePath the path to the Folder
	 * @param MountPoint The point in the VFS to "Mount" The folder (I.E. the starting point for adding files)
	 * @param Overwrite Whether to Overwrite the destination file if it already exists
	 * @param Regex The Regex string each filename must match to be added to the filesystem
	 */
	void addFolder(const std::filesystem::path& Folder, Path MountPoint, bool Overwrite = true, bool Recursive = false, const std::string Regex = "(.*)") {
		// Check to see if we're in the right directory of the VFS, otherwise pass to the next level
		if (MountPoint.totalDepth() > 0) {
			if (!folders.count(MountPoint[0])) {
				folders.emplace(MountPoint[0], new DatVFS());
			}
			folders[MountPoint[0]]->addFolder(Folder, MountPoint.getSubPath(1), Overwrite, Recursive, Regex);
		}
		else {
			addFolder(Folder, Overwrite, Recursive, Regex);
		}
	}

	/**
	 * Adds all the files in the given Archive to the VFS
	 * @param Archive The VFSArchive object containing the method to get VFSFile objects from the vfs
	 * @param Overwrite Whether to Overwrite the destination file if it already exists
	 */
	void addArchive(VFSArchive& Archive, bool Overwrite = false) {
		// Get all the files in the archive
		std::vector<std::pair<Path, DVFSFile*>> files = Archive.getFiles();

		for (std::pair<Path, DVFSFile*> file : files) {
			addFile(file.second, file.first, Overwrite);
		}
	}

	/**
	 * Adds all the files in the given Archive to the VFS
	 * @param Archive The VFSArchive object containing the method to get VFSFile objects from the vfs
	 * @param MountPoint The point in the VFS to "Mount" The Archive (I.E. the starting point for adding files)
	 * @param Overwrite Whether to Overwrite the destination file if it already exists
	 */
	void addArchive(VFSArchive& Archive, Path MountPoint, bool Overwrite = false) {
		// Check to see if we're in the write directory of the VFS, otherwise pass to the next level
		if (MountPoint.totalDepth() > 0) {
			if (!folders.count(MountPoint[0])) {
				folders.emplace(MountPoint[0], new DatVFS());
			}
			folders[MountPoint[0]]->addArchive(Archive, MountPoint.getSubPath(1), Overwrite);
		}
		else {
			addArchive(Archive, Overwrite);
		}
	}

	/**
	 * Counts all the files inside and below this directory in the VFS
	 * @return The amount of files inside and below this directory in the VFS
	 */
	int countFiles() {
		int count = files.size();
		for (std::unordered_map<std::string, DatVFS*>::iterator it = folders.begin(); it != folders.end(); ++it) {
			count += it->second->countFiles();
		}
		return count;
	}

	/**
	 * Counts all the files inside and below this directory in the VFS that match the given regex
	 * @param Regex The regex string the title must match
	 * @return The amount of files inside and below this directory in the VFS
	 */
	int countFilesMatchingRegex(const std::string Regex) {
		int count = 0;

		// Count files that match
		for (std::unordered_map<std::string, DVFSFile*>::iterator it = files.begin(); it != files.end();) {
			if (std::regex_match(it->first, std::regex(Regex))) {
				++count;
			}
		}

		// add count of each subdirectories
		for (std::unordered_map<std::string, DatVFS*>::iterator it = folders.begin(); it != folders.end();) {
			count += it->second->countFilesMatchingRegex(Regex);
		}

		return count;
	}

	/**
	 * Gets the file at the given location
	 * @param thePath The path to the file
	 * @return The file at the given location
	 */
	DVFSFile* getFile(Path thePath) {
		DVFSFile* file = getFileInternal(thePath);
		if (file == nullptr) throw MissingFileException((std::string) thePath);
		return file;
	}

	/**
	 * Gets the folder at the given location
	 * @param thePath The path of the folder
	 * @return The folder at the given location
	 */
	DatVFS* getFolder(Path thePath) {
		DatVFS* folder = getFolderInternal(thePath);
		if (folder == nullptr) throw MissingDirectoryException((std::string) thePath);
		return folder;
	}

	/**
	 * Gets a vector of all the files that match the given regex string inside this directory
	 * @param Regex The regex string to match the file title to
	 * @return A vector containing all the files that match the given regex string
	 */
	std::vector<DVFSFile*> getAllFilesThatMatchRegex(const std::string Regex) {
		// TODO: maybe use a linked list and only iterate through the vfs once
		int count = countFilesMatchingRegex(Regex);
		std::vector<DVFSFile*> Files(count);

		getAllFilesThatMatchRegexInternal(Regex, Files);
		return Files;
	}
	
	/**
	 * Removes all empty directories below this directory in the VFS
	 */
	void prune() {
		for (std::unordered_map<std::string, DatVFS*>::iterator it = folders.begin(); it != folders.end();) {
			it->second->prune();
			if (it->second->countFiles() == 0) {
				delete it->second;
				folders.erase(it++);
			}
			else {
				++it;
			}
		}	
	}

	/**
	 * Prints to cout all of the directories in the tree and all of the files in those directories in a formatted way
	 * @param Prefix The prefix of the file/folder when printing it (To add formatting)
	 * @param Depth The depth of the file/folder (To add formatting)
	 */
	void tree(std::string Prefix = "", int Depth = 0) {
		// Print folders and their subdirectories & files
		for (std::unordered_map<std::string, DatVFS*>::iterator it = folders.begin(); it != folders.end(); ++it) {
			std::cout << Prefix << (Depth != 0 ? "-" : "") << it->first << std::endl;
			it->second->tree(Prefix + " |", Depth + 1);
		}

		// Print Files
		for (std::unordered_map<std::string, DVFSFile*>::iterator it = files.begin(); it != files.end(); ++it) {
			std::cout << Prefix << (Depth != 0 ? "-" : "") << it->first << std::endl;
		}
	}
};