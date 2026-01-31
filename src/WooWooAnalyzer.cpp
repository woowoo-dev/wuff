//
// Created by Michal Janecek on 27.01.2024.
//

#include <filesystem>
#include <string>
#include <utility>

#include "WooWooAnalyzer.h"
#include "dialect/DialectManager.h"
#include "project/DialectedWooWooDocument.h"

#include "components/Hoverer.h"
#include "components/Highlighter.h"
#include "components/Navigator.h"
#include "components/Completer.h"
#include "components/Linter.h"
#include "components/Folder.h"

#include "utils/utils.h"

WooWooAnalyzer::WooWooAnalyzer() {
    highlighter = new Highlighter(this);
    hoverer = new Hoverer(this);
    navigator = new Navigator(this);
    completer = new Completer(this);
    linter = new Linter(this);
    folder = new Folder(this);
}

WooWooAnalyzer::~WooWooAnalyzer() {
    delete highlighter;
    delete hoverer;
    delete navigator;
    delete completer;
    delete linter;
    delete folder;

    for (auto &project: projects) {
        delete project;
    }
}

void WooWooAnalyzer::setDialect(const std::string &dialectPath) {
    DialectManager::getInstance()->loadDialect(dialectPath);
}


/**
 * Loads all WooWoo documents from the specified workspace URI.
 * 
 * This function converts the workspace URI to a local path and scans the directory
 * for project folders, loading any '.woo' files found within them. It also loads any
 * standalone '.woo' files that are not part of any project folder.
 * 
 * @param workspaceUri The URI of the workspace to load documents from.
 */
void WooWooAnalyzer::loadWorkspace(const std::string &workspaceUri) {
    // Convert URI to a local file system path
    workspaceRootPath = utils::uriToPathString(workspaceUri);

    // Find all project folders within the workspace
    auto projectFolders = findProjectFolders(workspaceRootPath);

    for (const fs::path &projectFolderPath: projectFolders) {
        projects.insert(new WooWooProject(projectFolderPath));
    }

    // Project for unassigned documents
    auto nullProject = new WooWooProject();
    projects.insert(nullProject);
    
    // Find and load all '.woo' files that are not part of any project
    auto woowooFiles = findAllWooFiles(workspaceRootPath);

    for (auto &woowooFile: woowooFiles) {
        bool partOfProject = false;
        for (auto &project: projects) {
            if (project->getDocument(woowooFile.generic_string())) {
                partOfProject = true;
            }
        }
        if (!partOfProject)
            nullProject->loadDocument(woowooFile);
    }

}


std::set<fs::path> WooWooAnalyzer::findAllWooFiles(const fs::path &rootPath) {
    std::set<fs::path> wooFiles;

    if (fs::exists(rootPath) && fs::is_directory(rootPath)) {
        for (const auto &entry: fs::recursive_directory_iterator(rootPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".woo") {
                wooFiles.insert(entry.path());
            }
        }
    }

    return wooFiles;
}

std::vector<fs::path> WooWooAnalyzer::findProjectFolders(const fs::path &rootPath) {

    std::vector<fs::path> projectFolders;
    for (const auto &entry: fs::recursive_directory_iterator(rootPath)) {
        if (entry.is_regular_file() && entry.path().filename() == "Woofile") {
            projectFolders.push_back(entry.path().parent_path());
        }
    }
    return projectFolders;
}

std::optional<fs::path> WooWooAnalyzer::findProjectFolder(const std::string &uri) {
    fs::path path = utils::uriToPathString(uri);

    // Start from parent of the file
    fs::path parent = path.parent_path();

    // Explore up to the root folder of the project
    while (parent != workspaceRootPath.parent_path() && parent != parent.parent_path()) {
        fs::path woofilePath = parent / "Woofile";

        // Check if Woofile exists in this directory
        if (fs::exists(woofilePath)) {
            return parent; // Return the parent directory containing Woofile
        }
        parent = parent.parent_path();
    }

    return std::nullopt; // no project folder found
}

DialectedWooWooDocument * WooWooAnalyzer::getDocumentByUri(const std::string &docUri) {
    auto path = utils::uriToPathString(docUri);
    return getDocument(path);
}

DialectedWooWooDocument * WooWooAnalyzer::getDocument(const std::string &pathToDoc) {

    for (auto &project: projects) {
        auto doc = project->getDocument(pathToDoc);
        if (doc)
            return doc;
    }

    return nullptr;
}


WooWooProject *WooWooAnalyzer::getProjectByDocument(WooWooDocument * document) {

    for (auto &project: projects) {
        auto doc = project->getDocument(document);
        if (doc)
            return project;
    }

    return nullptr;
}


void WooWooAnalyzer::handleDocumentChange(const TextDocumentIdentifier &tdi, std::string &source) {
    auto docPath = utils::uriToPathString(tdi.uri);
    auto document = getDocument(docPath);
    if (document) {
        document->updateSource(source);
    }
}

/**
 * Handles renaming of files within the workspace and updates internal mappings and references.
 * This function processes a list of file renames, updating the document paths and project associations.
 * It supports renaming '.woo' files within their respective projects or to new locations, and also handles
 * the cleanup of documents no longer recognized as '.woo' files after the rename.
 *
 * @param renames A list of pairs representing old and new URIs for the files being renamed.
 * @return A WorkspaceEdit object that details the changes made to document references.
 */
WorkspaceEdit WooWooAnalyzer::renameFiles(const std::vector<std::pair<std::string, std::string>> &renames) {
    WorkspaceEdit we;

    std::vector<std::pair<std::string, std::string>> renamedDocuments;
    for (const auto &fileRename: renames) {

        auto oldUri = fileRename.first;
        auto newUri = fileRename.second;
        auto oldPath = utils::uriToPathString(oldUri);
        auto newPath = utils::uriToPathString(newUri);

        if (utils::endsWith(oldPath, ".woo") && utils::endsWith(newPath, ".woo")) {
            // Handle renaming of WooWoo files within the same or to a different project
            auto document = getDocument(oldPath);
            if (!document) continue;
            auto oldProject = getProjectByDocument(document);
            if (!oldProject) continue;
            auto documentShared = oldProject->getDocumentShared(document);

            std::optional<fs::path> newProjectFolder = findProjectFolder(newUri);
            auto newProject = getProject(newProjectFolder);
            if (!newProject) {
                // Fall back to null project
                newProject = getProject(std::nullopt);
            }
            if (newProject) {
                document->documentPath = newPath;
                oldProject->deleteDocument(document);
                newProject->addDocument(documentShared);
            }

            renamedDocuments.emplace_back(oldPath, newPath);

        } else if (utils::endsWith(oldPath, ".woo")) {
            // Handle the case where a '.woo' document is renamed to a non-WooWoo format
            deleteDocument(oldPath);
        } else {
            // Handle renaming of non-WooWoo files or conversion of non-WooWoo to '.woo' files
            // These are handled elsewhere as new documents through openDocument
        }
    }

    // After updating the internal state, refactor the document references to reflect the new file paths
    return navigator->refactorDocumentReferences(renamedDocuments);
}


/**
 * Processes deletions of files as notified by the client. This function currently handles the deletion
 * of individual document files by removing them from the internal state and associated data structures.
 * It does not handle the deletion of folders or Woofile files yet.
 *
 * @param uris A list of URIs for the files that have been deleted.
 */
void WooWooAnalyzer::didDeleteFiles(const std::vector<std::string> &uris) {
    for (const auto &deletedFileUri: uris) {

        auto doc = getDocumentByUri(deletedFileUri);
        if (doc) {
            deleteDocument(doc);
        }
    }
    // NOTE: This function does not yet handle the deletion of folders or Woofiles.
}


void WooWooAnalyzer::deleteDocument(const std::string &uri) {
    auto doc = getDocumentByUri(uri);
    deleteDocument(doc);
}

void WooWooAnalyzer::deleteDocument(DialectedWooWooDocument * document) {
    if (!document) return;
    for (auto project: projects) {
        project->deleteDocument(document);
    }
}


// - LSP-like public interface - - -

std::string WooWooAnalyzer::hover(const TextDocumentPositionParams &params) {
    return hoverer->hover(params);
}

std::vector<int> WooWooAnalyzer::semanticTokens(const TextDocumentIdentifier &tdi) {
    return highlighter->semanticTokens(tdi);
}

Location WooWooAnalyzer::goToDefinition(const DefinitionParams &params) {
    return navigator->goToDefinition(params);
}

std::vector<Location> WooWooAnalyzer::references(const ReferenceParams &params) {
    return navigator->references(params);
}

WorkspaceEdit WooWooAnalyzer::rename(const RenameParams &params) {
    return navigator->rename(params);
}

std::vector<CompletionItem> WooWooAnalyzer::complete(const CompletionParams &params) {
    return completer->complete(params);
}

std::vector<FoldingRange> WooWooAnalyzer::foldingRanges(const TextDocumentIdentifier &tdi) {
    return folder->foldingRanges(tdi);
}

void WooWooAnalyzer::documentDidChange(const TextDocumentIdentifier &tdi, std::string &source) {
    handleDocumentChange(tdi, source);
}

std::vector<Diagnostic> WooWooAnalyzer::diagnose(const TextDocumentIdentifier &tdi) {
    return linter->diagnose(tdi);
}

void WooWooAnalyzer::openDocument(const TextDocumentIdentifier &tdi) {
    auto docPath = utils::uriToPathString(tdi.uri);
    if (!getDocument(docPath)) {
        // unknown document opened
        std::optional<fs::path> projectFolder = findProjectFolder(tdi.uri);
        auto project = getProject(projectFolder);
        if (!project) {
            // Fall back to null project for standalone files
            project = getProject(std::nullopt);
        }
        if (project) {
            project->loadDocument(docPath);
        }
    }
}

WooWooProject *WooWooAnalyzer::getProject(const std::optional<fs::path> &path) {
    for (auto project : projects){
        if(project->projectFolderPath == path){
            return project;
        }
    }
    return nullptr;
}

void WooWooAnalyzer::setTokenTypes(std::vector<std::string> tokenTypes) {
    return highlighter->setTokenTypes(std::move(tokenTypes));
}

void WooWooAnalyzer::setTokenModifiers(std::vector<std::string> tokenModifiers) {
    return highlighter->setTokenModifiers(std::move(tokenModifiers));
}

// - - - - - - - - - - - - - - - - -