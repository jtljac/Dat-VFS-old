#pragma once

#include <cstdint>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <cassert>
#include <regex>
#include "DatVFS/DatVFSCommon.h"

class DatVFS {
    std::unordered_map<std::string, DatVFS*> folders;
    std::unordered_map<std::string, IDVFSFile*> files;

public:
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
     * @param Regex The regex string the title must match
     * @return The amount of files inside and below this directory in the VFS
     */
    int countFilesMatchingRegex(const std::string& Regex) {
        int count = 0;

        // Count files that match
        for (auto& file: files) {
            if (std::regex_match(file.first, std::regex(Regex))) {
                ++count;
            }
        }

        // add count of each subdirectory
        for (auto& folder: folders) {
            count += folder.second->countFilesMatchingRegex(Regex);
        }

        return count;
    }

    /**
     * Gets the file at the given location
     * @param thePath The path to the file
     * @return The file at the given location
     */
    IDVFSFile* getFile(DVFSPath thePath) {
        IDVFSFile* file = getFiledInternal(thePath);
        return file;
    }

    /**
     * Gets the folder at the given location
     * @param thePath The path of the folder
     * @return The folder at the given location
     */
    DatVFS* getFolder(Path thePath) {
        DatVFS* folder = getFolderInternal(thePath);
        return folder;
    }

    /**
     * Gets a vector of all the files that match the given regex string inside this directory
     * @param Regex The regex string to match the file title to
     * @return A vector containing all the files that match the given regex string
     */
    std::vector<IDVFSFile*> getAllFilesThatMatchRegex(const std::string& Regex) {
        // TODO: maybe use a linked list and only iterate through the vfs once
        int count = countFilesMatchingRegex(Regex);
        std::vector<IDVFSFile*> Files(count);

        getAllFilesThatMatchRegexInternal(Regex, Files);
        return Files;
    }

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
        // Print folders and their subdirectories & files
        for (auto& folder: folders) {
            std::cout << Prefix << (Depth != 0 ? "-" : "") << folder.first << std::endl;
            folder.second->tree(Prefix + " |", Depth + 1);
        }

        // Print Files
        for (auto& file: files) {
            std::cout << Prefix << (Depth != 0 ? "-" : "") << file.first << std::endl;
        }
    }
};