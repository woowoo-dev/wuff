#include <utility>
#include <optional>

//
// Created by Michal Janecek on 31.01.2024.
//

#ifndef WUFF_LSPTYPES_H
#define WUFF_LSPTYPES_H

struct Position {
    uint32_t line;
    uint32_t character;
};

struct Range {
    Position start;
    Position end;
};

struct Location {
    std::string uri;
    Range range;

    Location(std::string uri, Range range) : uri(std::move(uri)), range(range) {}
};

struct TextDocumentIdentifier {
    std::string uri;

    explicit TextDocumentIdentifier(std::string uri) : uri(std::move(uri)) {}
};

struct TextDocumentPositionParams {
    TextDocumentIdentifier textDocument;
    Position position;

    TextDocumentPositionParams(TextDocumentIdentifier textDocument, Position position)
            : textDocument(std::move(textDocument)), position(position) {}
};

struct DefinitionParams : public TextDocumentPositionParams {
    using TextDocumentPositionParams::TextDocumentPositionParams; // Inherit constructors
};


enum class CompletionTriggerKind {
    Invoked = 1,
    TriggerCharacter = 2,
    TriggerForIncompleteCompletions = 3,
};

struct CompletionContext {
    CompletionTriggerKind triggerKind;
    std::optional<std::string> triggerCharacter;

    explicit CompletionContext(CompletionTriggerKind triggerKind,
                               std::optional<std::string> triggerCharacter = std::nullopt)
            : triggerKind(triggerKind), triggerCharacter(std::move(triggerCharacter)) {}
};


struct CompletionParams : public TextDocumentPositionParams {
    std::optional<CompletionContext> context; // Context is optional

    CompletionParams(const TextDocumentIdentifier &textDocument, const Position &position,
                     std::optional<CompletionContext> context = std::nullopt)
            : TextDocumentPositionParams(textDocument, position), context(std::move(context)) {}
};

enum class CompletionItemKind {
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Unit = 11,
    Value = 12,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Color = 16,
    File = 17,
    Reference = 18,
    Folder = 19,
    EnumMember = 20,
    Constant = 21,
    Struct = 22,
    Event = 23,
    Operator = 24,
    TypeParameter = 25
};

enum class InsertTextFormat {
    PlainText = 1,
    Snippet = 2
};

struct TextEdit {
    Range range;

    std::string newText;

    TextEdit(Range range, std::string newText)
            : range(std::move(range)), newText(std::move(newText)) {}
};

struct CompletionItem {
    std::string label;
    std::optional<CompletionItemKind> kind;
    std::optional<InsertTextFormat> insertTextFormat;
    std::optional<std::string> insertText;
    std::optional<std::string> detail;
    // Markdown
    std::optional<std::string> documentation;
    std::optional<TextEdit> textEdit;

    explicit CompletionItem(std::string label, std::optional<CompletionItemKind> kind = std::nullopt,
                            std::optional<InsertTextFormat> insertTextFormat = std::nullopt,
                            std::optional<std::string> insertText = std::nullopt,
                            std::optional<std::string> detail = std::nullopt,
                            std::optional<std::string> documentation = std::nullopt,
                            std::optional<TextEdit> textEdit = std::nullopt)
            : label(std::move(label)), kind(kind), insertTextFormat(insertTextFormat),
              insertText(std::move(insertText)), detail(std::move(detail)),
              documentation(std::move(documentation)), textEdit(std::move(textEdit)) {}
};


struct ReferenceParams: public TextDocumentPositionParams {
    bool includeDeclaration;

    ReferenceParams(const TextDocumentIdentifier &textDocument, const Position &position, bool includeDeclaration)
            : TextDocumentPositionParams(textDocument, position), includeDeclaration(std::move(includeDeclaration)) {}
    
};

struct RenameParams : public TextDocumentPositionParams {
    std::string newName;

    RenameParams(const TextDocumentIdentifier &textDocument, const Position &position, std::string newName)
            : TextDocumentPositionParams(textDocument, position), newName(std::move(newName)) {}
};


struct WorkspaceEdit {
    // The key is the file URI
    std::unordered_map<std::string, std::vector<TextEdit>> changes;

    void add_change(const std::string& uri, const TextEdit& textEdit) {
        changes[uri].push_back(textEdit);
    }
};

enum class DiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4
};

struct Diagnostic {
    Range range;
    std::string message;
    std::string source;
    DiagnosticSeverity severity;

    Diagnostic(Range range, std::string message, std::string source, DiagnosticSeverity severity)
            : range(range), message(std::move(message)), source(std::move(source)), severity(severity) {}
};


struct FoldingRange {
    uint32_t startLine;
    uint32_t startCharacter;
    uint32_t endLine;
    uint32_t endCharacter;
    std::string foldingRangeKind;

    FoldingRange(uint32_t startLine, uint32_t startCharacter, uint32_t endLine, uint32_t endCharacter,
                 std::string foldingRangeKind) : startLine(startLine), startCharacter(startCharacter),
                                                 endLine(endLine), endCharacter(endCharacter),
                                                 foldingRangeKind(std::move(foldingRangeKind)) {}
};

#endif //WUFF_LSPTYPES_H
