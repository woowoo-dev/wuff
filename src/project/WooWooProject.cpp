//
// Created by Michal Janecek on 16.06.2024.
//

#include "WooWooProject.h"
#include "DialectedWooWooDocument.h"
#include "../utils/utils.h"


WooWooProject::WooWooProject() : projectFolderPath(std::nullopt) {}

WooWooProject::WooWooProject(const fs::path &projectFolderPath) : projectFolderPath(projectFolderPath) {

    // Woofile parsing disabled for now - just detect project by Woofile existence
    // TODO: Enable when Woofile features (bibtex, etc.) are implemented
    woofile = nullptr;

    for (const auto &entry: fs::recursive_directory_iterator(projectFolderPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".woo") {
            loadDocument(entry.path());
        }
    }

}


void WooWooProject::loadDocument(const fs::path &documentPath) {
    documents[documentPath.generic_string()] = std::make_shared<DialectedWooWooDocument>(documentPath);
}

void WooWooProject::addDocument(const std::shared_ptr<DialectedWooWooDocument>& document) {
    documents[document->documentPath.generic_string()] = document;
}

DialectedWooWooDocument * WooWooProject::getDocument(const std::string &docPath) {
    auto doc = documents.find(docPath);
    if (doc != documents.end()) {
        return doc->second.get();
    }
    return nullptr;
}

DialectedWooWooDocument * WooWooProject::getDocument(const WooWooDocument*  document) {
    for (const auto &doc: documents) {
        if (doc.second->documentPath == document->documentPath) {
            return doc.second.get();
        }
    }
    return nullptr;
}

std::set<DialectedWooWooDocument * > WooWooProject::getAllDocuments() {
    std::set<DialectedWooWooDocument * > documentSet;
    for (const auto &pair: documents) {
        documentSet.insert(pair.second.get());
    }
    return documentSet;
}


DialectedWooWooDocument * WooWooProject::getDocumentByUri(const std::string &docUri) {
    auto path = utils::uriToPathString(docUri);
    return getDocument(path);
}

void WooWooProject::deleteDocumentByUri(const std::string &uri) {
    auto doc = getDocumentByUri(uri);
    deleteDocument(doc);
}

std::shared_ptr<DialectedWooWooDocument> WooWooProject::getDocumentShared(WooWooDocument *document) {
    for (const auto &doc: documents) {
        if (doc.second->documentPath == document->documentPath) {
            return doc.second;
        }
    }
    return nullptr;
}

void WooWooProject::deleteDocument(const DialectedWooWooDocument * document) {
    if (!document) return;
    auto it = documents.find(document->documentPath.generic_string());
    if (it != documents.end()) {
        documents.erase(it);
    }
}
