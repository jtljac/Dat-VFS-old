#pragma once

#include <cstdint>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <cassert>
#include <regex>
#include "DatVFS/DatVFSCommon.h"

class DatVFS {
    using FolderMap = std::unordered_map<std::string,DatVFS*>;
    using FileMap = std::unordered_map<std::string,IDVFSFile*>;

    FolderMap folders;
    FileMap files;

public:
    DatVFS() {
        folders["."] = this;
        // Since we don't know who the parent is, we'll just have to point .. at itself aswell
        folders[".."] = this;
    }

    explicit DatVFS(DatVFS* parent) {
        folders["."] = this;
        folders[".."] = parent;
    }

    /**
     * Counts all the files inside and below this directory in the VFS
     * @return The amount of files inside and below this directory in the VFS
     */
    size_t countFiles() {
        size_t count = files.size();
        for (auto& folder: folders) {
            count += folder.second->countFiles();
        }
        return count;
    }

    /**
     * Counts all the files inside and below this directory in the VFS that match the given regex
     * @param regex The regex string the title must match
     * @return The amount of files inside and below this directory in the VFS
     */
    inline int countFilesMatchingRegex(const std::string& regex) {
        return countFilesMatchingRegex(std::regex(regex));
    }

    /**
     * Counts all the files inside and below this directory in the VFS that match the given regex
     * @param regex The regex the title must match
     * @return The amount of files inside and below this directory in the VFS
     */
    int countFilesMatchingRegex(const std::regex& regex) {
        int count = 0;

        // Count files that match
        for (auto& file: files) {
            if (std::regex_match(file.first, regex)) {
                ++count;
            }
        }

        // add count of each subdirectory
        for (auto& folder: folders) {
            count += folder.second->countFilesMatchingRegex(regex);
        }

        return count;
    }

    /**
     * Retrieves the file at the given path
     * @param filePath The path to the file
     * @param index (Optional) The index of the path to start from
     * @return The file at the given location, null if no file is found
     */
    IDVFSFile* getFile(const std::vector<std::string>& filePath, size_t index = 0) {
        if (index == filePath.size() - 1) {
            return files[filePath[index]];
        } else if (index < filePath.size()) {
            FolderMap::iterator folderIt = folders.find(filePath[index]);
            DatVFS* folder;
            if (folderIt == folders.end()) {
                return nullptr;
            } else {
                folder = folderIt->second;
            }
            return folder->getFile(filePath, index + 1);
        } else {
            return nullptr;
        }
    }

    /**
     * Retrieves the file at the given path
     * @param filePath The path to the file
     * @return The file at the given location, null if no file is found
     */
    IDVFSFile* getFile(const std::string& filePath) {
        std::vector<std::string> filePathList = stringPathToVectorPath(filePath);

        return getFile(filePathList, 0);
    }

    /**
     * Retrieves the folder at the given path
     * @param folderPath The path of the folder
     * @param index (Optional) The index of the path to start from
     * @return The folder at the given location
     */
    DatVFS* getFolder(const std::vector<std::string>& folderPath, size_t index = 0) {
        if (index == folderPath.size() - 1) {
            return folders[folderPath[index]];
        } else if (index < folderPath.size()) {
            FolderMap::iterator folderIt = folders.find(folderPath[index]);
            DatVFS* folder;
            if (folderIt == folders.end()) {
                return nullptr;
            } else {
                folder = folderIt->second;
            }

            return folders[folderPath[index]]->getFolder(folderPath, index + 1);
        } else {
            return nullptr;
        }
    }

    /**
     * Retrieves the folder at the given path
     * @param folderPath The path of the folder
     * @param index (Optional) The index of the path to start from
     * @return The folder at the given location
     */
    DatVFS* getFolder(const std::string& folderPath) {
        std::vector<std::string> folderPathList = stringPathToVectorPath(folderPath);

        return getFolder(folderPathList, 0);
    }

    /**
     * Creates a folder within the current directory
     * @param folderName The name of the folder (cannot contain backslashes or forward slashes)
     * @return The newly created folder
     */
    DatVFS* createSingleFolder(const std::string& folderName) {
        if (folders.count(folderName) > 0 || std::count_if(folderName.begin(), folderName.end(), [&](const auto& item) {
            return item == '\\' || item == '/';
        }) > 0) {
            return nullptr;
        }

        DatVFS* newFolder = new DatVFS(this);
        folders.emplace(std::string(folderName), newFolder);
        return newFolder;
    }

    /**
     * Creates a folder at the given path
     * @param folderPath The path of the folder being created
     * @param recursive (Optional) If folders that don't exist leading up to the last folder should be created
     * @param index (Optional) The index of the path to start from
     * @return The newly created folder (nullptr if the creation failed)
     */
    DatVFS* createFolder(const std::vector<std::string>& folderPath, bool recursive = false, size_t index = 0) {
        if (index == folderPath.size() - 1) {
            createSingleFolder(folderPath[index]);
        } else if (index < folderPath.size()) {
            FolderMap::iterator folderIt = folders.find(folderPath[index]);
            DatVFS* folder;
            if (folderIt == folders.end()) {
                if (recursive) {
                    folder = createSingleFolder(folderPath[index]);
                } else {
                    return nullptr;
                }
            } else {
                folder = folderIt->second;
            }
            return folder->createFolder(folderPath, index + 1);
        } else {
            return nullptr;
        }
    }

    /**
     * Creates a folder at the given path
     * @param folderPath The path of the folder being created
     * @param recursive If folders that don't exist leading up to the last folder should be created
     * @return The newly created folder (nullptr if the creation failed)
     */
    DatVFS* createFolder(const std::string& folderPath, bool recursive = false) {
        std::vector<std::string> folderPathList = stringPathToVectorPath(folderPath);

        return createFolder(folderPathList, recursive, 0);
    }

    /**
     * Inserts the IDVFSFile into VFS
     * If there is already a file there, then it will be overwritten
     * @param filePath The path to the file
     * @param dvfsFile The file to insert
     * @param pathIndex (Optional) The index of the path to start from
     * @return If the file was successfully inserted
     */
    bool insertFile(const std::vector<std::string>& filePath, IDVFSFile* dvfsFile, bool createFolders = true, size_t pathIndex = 0) {
        if (pathIndex == filePath.size() - 1) {
            if (files.count(filePath[pathIndex]) > 0) {
                delete files[filePath[pathIndex]];
            }
            files[filePath[pathIndex]] = dvfsFile;
            return true;
        } else if (pathIndex < filePath.size()) {
            FolderMap::iterator folderIt = folders.find(filePath[pathIndex]);
            DatVFS* folder;
            if (folderIt == folders.end()) {
                if (createFolders) {
                    folder = createSingleFolder(filePath[pathIndex]);
                } else {
                    return false;
                }
            } else {
                folder = folderIt->second;
            }
            return folder->insertFile(filePath, dvfsFile, createFolders, pathIndex + 1);
        } else {
            return false;
        }
    }

    /**
     * Inserts the IDVFSFile into VFS
     * @param filePath The path to the file
     * @param dvfsFile The file to insert
     * @return If the file was successfully inserted
     */
    bool insertFile(const std::string& filePath, IDVFSFile* dvfsFile, bool createFolders = true) {
        std::vector<std::string> filePathList = stringPathToVectorPath(filePath);

        return insertFile(filePathList, dvfsFile, createFolders ,0);
    }

    /**
     * Inserts the files defined by the inserter into the VFS
     * @param inserter The inserter defining the files to insert
     * @param mountIndex (Optional) The index of the mount path to start from
     * @return If the file was successfully inserted
     */
    bool insertFiles(const IDVFSInserter& inserter, size_t mountIndex = 0) {
        if (!inserter.mountPoint.empty() && mountIndex < inserter.mountPoint.size() - 1) {
            DatVFS* folder = folders[inserter.mountPoint[mountIndex]];

            if (folder) return folder->insertFiles(inserter, mountIndex + 1);
            else return false;
        }

        for (const auto& item : inserter.getAllFiles()) {
            insertFile(item.first, item.second, true);
        }
    }

//    /**
//     * Gets a vector of all the files that match the given regex string inside this directory
//     * @param Regex The regex string to match the file title to
//     * @return A vector containing all the files that match the given regex string
//     */
//    std::vector<IDVFSFile*> getAllFilesThatMatchRegex(const std::string& Regex) {
//        // TODO: maybe use a linked list and only iterate through the vfs once
//        int count = countFilesMatchingRegex(Regex);
//        std::vector<IDVFSFile*> Files(count);
//
//        getAllFilesThatMatchRegexInternal(Regex, Files);
//        return Files;
//    }

    /**
     * Removes all empty directories below this directory in the VFS
     */
    void prune() {
        for (auto it = folders.begin(); it != folders.end();) {
            it->second->prune();
            if (it->second->countFiles() == 0) {
                delete it->second;
                folders.erase(it++);
            } else {
                ++it;
            }
        }
    }

    /**
     * Prints to cout all of the directories in the tree and all of the files in those directories in a formatted way
     * @param Prefix The prefix of the file/folder when printing it (To add formatting)
     * @param Depth The depth of the file/folder (To add formatting)
     */
    void tree(const std::string& Prefix = "", int Depth = 0) {
        // Always print . and .. first
        std::cout << Prefix << (Depth != 0 ? "-" : "") << "." << "/" << std::endl;
        std::cout << Prefix << (Depth != 0 ? "-" : "") << ".." << "/" << std::endl;

        // Print folders and their subdirectories & files
        for (auto& folder: folders) {
            if (!(folder.first == "." || folder.first == "..")) {
                std::cout << Prefix << (Depth != 0 ? "-" : "") << folder.first << "/" << std::endl;
                folder.second->tree(Prefix + " |", Depth + 1);
            }
        }

        // Print Files
        for (auto& file: files) {
            std::cout << Prefix << (Depth != 0 ? "-" : "") << file.first << std::endl;
        }
    }
};