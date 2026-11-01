//
// Created by Michal Janecek on 16.06.2024.
//

#include "WooWooProject.h"
#include "DialectedWooWooDocument.h"
#include "../utils/utils.h"


WooWooProject::WooWooProject() : projectFolderPath(std::nullopt) {}

WooWooProject::WooWooProject(const fs::path &projectFolderPath) : projectFolderPath(projectFolderPath) {

    refreshWoofile();

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

namespace {

Referencable referencableOf(const Bibliography &bibliography, const BibliographyEntry &entry) {
    auto summary = entry.summary();
    return {entry.key, Location(utils::pathToUri(bibliography.path()), entry.keyRange), summary, summary};
}

std::optional<fs::file_time_type> modificationTime(const fs::path &path) {
    std::error_code error;
    auto time = fs::last_write_time(path, error);
    if (error) return std::nullopt;
    return time;
}

} // namespace

void WooWooProject::refreshWoofile() {
    if (!projectFolderPath.has_value()) return;
    auto time = modificationTime(projectFolderPath.value() / "Woofile");
    if (woofileRead && time == woofileTime) return;
    woofile = Woofile::load(projectFolderPath.value());
    woofileTime = time;
    woofileRead = true;
}

const Bibliography *WooWooProject::getBibliography() {
    refreshWoofile();
    if (!woofile.bibtex.has_value()) {
        bibliography.reset();
        bibliographyPath.reset();
        return nullptr;
    }

    auto time = modificationTime(woofile.bibtex.value());
    if (bibliographyPath != woofile.bibtex || bibliographyTime != time) {
        bibliography = Bibliography::load(woofile.bibtex.value());
        bibliographyPath = woofile.bibtex;
        bibliographyTime = time;
    }
    return bibliography.get();
}

std::vector<Referencable> WooWooProject::getReferencables(const Reference &reference) {
    std::vector<Referencable> referencables;
    if (reference.target == ReferenceTarget::MetaField) {
        for (auto document: getAllDocuments()) {
            auto found = document->getReferencables(reference);
            referencables.insert(referencables.end(), found.begin(), found.end());
        }
    } else if (reference.target == ReferenceTarget::Bibliography) {
        if (auto bib = getBibliography()) {
            for (const BibliographyEntry &entry: bib->entries()) {
                referencables.push_back(referencableOf(*bib, entry));
            }
        }
    }
    return referencables;
}

std::optional<Referencable> WooWooProject::resolve(const std::vector<Reference> &references, const std::string &value) {
    for (const Reference &reference: references) {
        if (reference.target == ReferenceTarget::MetaField) {
            for (auto document: getAllDocuments()) {
                auto found = document->findReferencable(reference, value);
                if (found.has_value()) return found;
            }
        } else if (reference.target == ReferenceTarget::Bibliography) {
            auto bib = getBibliography();
            const BibliographyEntry *entry = bib ? bib->find(value) : nullptr;
            if (entry) return referencableOf(*bib, *entry);
        }
    }
    return std::nullopt;
}

std::optional<std::string> WooWooProject::describeReferencables(const Reference &reference) {
    if (reference.target == ReferenceTarget::Bibliography) {
        auto bib = getBibliography();
        if (bib) return "the bibliography (" + bib->path().filename().string() + ")";
    }
    return std::nullopt;
}
