//
// Created by Michal Janecek on 16.06.2024.
//

#ifndef WUFF_WOOWOOPROJECT_H
#define WUFF_WOOWOOPROJECT_H
#include <string>
#include <unordered_map>
#include <filesystem>
#include "DialectedWooWooDocument.h"
#include "Woofile.h"
#include "Bibliography.h"
#include "Referencable.h"

namespace fs = std::filesystem;


class WooWooProject {

private:
    std::unordered_map<std::string, std::shared_ptr<DialectedWooWooDocument>> documents;

    // the Woofile and the bibliography are read again when their modification time changes
    Woofile woofile;
    std::optional<fs::file_time_type> woofileTime;
    bool woofileRead = false;
    std::unique_ptr<Bibliography> bibliography;
    std::optional<fs::path> bibliographyPath;
    std::optional<fs::file_time_type> bibliographyTime;
    void refreshWoofile();

public:
    std::optional<fs::path> projectFolderPath;
    WooWooProject();
    WooWooProject(const fs::path & projectFolderPath);
    DialectedWooWooDocument * getDocument(const std::string & docPath);
    DialectedWooWooDocument * getDocument(const WooWooDocument * document);
    DialectedWooWooDocument * getDocumentByUri(const std::string &docUri);
    std::shared_ptr<DialectedWooWooDocument> getDocumentShared(WooWooDocument * doc);
    std::set<DialectedWooWooDocument *> getAllDocuments();  // New member function declaration
    void deleteDocumentByUri(const std::string &uri);
    void loadDocument(const fs::path &documentPath);
    void deleteDocument(const DialectedWooWooDocument * document);
    void addDocument(const std::shared_ptr<DialectedWooWooDocument>& document);

    // nullptr without a project folder, builder.bibtex in the Woofile, or a readable .bib file
    const Bibliography * getBibliography();

    // Reference resolution: one place that knows what each reference target means.
    // everything the reference can point at in this project
    std::vector<Referencable> getReferencables(const Reference & reference);
    // what the value points at through any of the references
    std::optional<Referencable> resolve(const std::vector<Reference> & references, const std::string & value);
    // where the referencables of the reference come from, for messages (e.g. "the bibliography (references.bib)");
    // nullopt when they are not available
    std::optional<std::string> describeReferencables(const Reference & reference);
};


#endif //WUFF_WOOWOOPROJECT_H
