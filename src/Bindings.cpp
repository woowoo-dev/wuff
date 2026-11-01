//
// Created by Michal Janecek on 27.01.2024.
//


#include <pybind11/pybind11.h>            // Core functionality
#include <pybind11/stl.h>                 // STL container bindings
#include <pybind11/iostream.h>            // Redirecting C++ streams to Python

#include "WooWooAnalyzer.h"

namespace py = pybind11;

PYBIND11_MODULE(wuff, m) {
    py::class_<WooWooAnalyzer>(m, "WooWooAnalyzer")
            .def(py::init<>())
            .def("set_dialect", &WooWooAnalyzer::setDialect)
            .def("load_workspace", &WooWooAnalyzer::loadWorkspace)
            .def("hover", &WooWooAnalyzer::hover)
            .def("semantic_tokens", &WooWooAnalyzer::semanticTokens)
            .def("go_to_definition", &WooWooAnalyzer::goToDefinition)
            .def("complete", &WooWooAnalyzer::complete)
            .def("references", &WooWooAnalyzer::references)
            .def("rename", &WooWooAnalyzer::rename)
            .def("folding_ranges", &WooWooAnalyzer::foldingRanges)
            .def("document_did_change", &WooWooAnalyzer::documentDidChange)
            .def("open_document", &WooWooAnalyzer::openDocument)
            .def("rename_files", &WooWooAnalyzer::renameFiles)
            .def("did_delete_files", &WooWooAnalyzer::didDeleteFiles)
            .def("diagnose", &WooWooAnalyzer::diagnose)
            .def("set_token_types", &WooWooAnalyzer::setTokenTypes)
            .def("set_token_modifiers", &WooWooAnalyzer::setTokenModifiers);


    py::class_<Position>(m, "Position")
            .def(py::init<uint32_t, uint32_t>())
            .def_readwrite("line", &Position::line)
            .def_readwrite("character", &Position::character);

    py::class_<Range>(m, "Range")
            .def(py::init<Position, Position>())
            .def_readwrite("start", &Range::start)
            .def_readwrite("end", &Range::end);

    py::class_<Location>(m, "Location")
            .def(py::init<std::string, Range>())
            .def_readwrite("uri", &Location::uri)
            .def_readwrite("range", &Location::range);

    py::enum_<CompletionTriggerKind>(m, "CompletionTriggerKind")
            .value("Invoked", CompletionTriggerKind::Invoked)
            .value("TriggerCharacter", CompletionTriggerKind::TriggerCharacter)
            .value("TriggerForIncompleteCompletions", CompletionTriggerKind::TriggerForIncompleteCompletions)
            .export_values();
    

    py::class_<TextDocumentIdentifier>(m, "TextDocumentIdentifier")
            .def(py::init<const std::string&>())
            .def_readwrite("uri", &TextDocumentIdentifier::uri);


    py::class_<TextDocumentPositionParams>(m, "TextDocumentPositionParams")
            .def(py::init<TextDocumentIdentifier, Position>())
            .def_readwrite("text_document", &TextDocumentPositionParams::textDocument)
            .def_readwrite("position", &TextDocumentPositionParams::position);

    // just for clarity - internally, it is the same as TextDocumentPositionParams
    py::class_<DefinitionParams, TextDocumentPositionParams>(m, "DefinitionParams")
            .def(py::init<TextDocumentIdentifier, Position>());

   

    py::class_<CompletionContext>(m, "CompletionContext")
            .def(py::init<CompletionTriggerKind, std::optional<std::string>>())
            .def_readwrite("trigger_kind", &CompletionContext::triggerKind)
            .def_readwrite("trigger_character", &CompletionContext::triggerCharacter);

    py::class_<CompletionParams, TextDocumentPositionParams>(m, "CompletionParams")
            .def(py::init<const TextDocumentIdentifier&, const Position&, std::optional<CompletionContext>>())
            .def_readwrite("context", &CompletionParams::context);

    py::enum_<CompletionItemKind>(m, "CompletionItemKind")
            .value("Text", CompletionItemKind::Text)
            .value("Snippet", CompletionItemKind::Snippet)
            .value("Reference", CompletionItemKind::Reference)
             // Note: LSP lists many other possible values, which are not used in this project right now.
            .export_values();

    py::enum_<InsertTextFormat>(m, "InsertTextFormat")
            .value("PlainText", InsertTextFormat::PlainText)
            .value("Snippet", InsertTextFormat::Snippet)
            .export_values();

    py::class_<TextEdit>(m, "TextEdit")
            .def(py::init<Range, std::string>())
            .def_readwrite("range", &TextEdit::range)
            .def_readwrite("new_text", &TextEdit::newText);

    py::class_<CompletionItem>(m, "CompletionItem")
            .def(py::init<std::string, std::optional<CompletionItemKind>,
                         std::optional<InsertTextFormat>, std::optional<std::string>,
                         std::optional<std::string>, std::optional<std::string>, std::optional<TextEdit>>(),
                 py::arg("label"), py::arg("kind") = std::nullopt,
                 py::arg("insertTextFormat") = std::nullopt, py::arg("insertText") = std::nullopt,
                 py::arg("detail") = std::nullopt, py::arg("documentation") = std::nullopt,
                 py::arg("text_edit") = std::nullopt)
            .def_readwrite("label", &CompletionItem::label)
            .def_readwrite("kind", &CompletionItem::kind)
            .def_readwrite("insertTextFormat", &CompletionItem::insertTextFormat)
            .def_readwrite("insertText", &CompletionItem::insertText)
            .def_readwrite("detail", &CompletionItem::detail)
            .def_readwrite("documentation", &CompletionItem::documentation)
            .def_readwrite("text_edit", &CompletionItem::textEdit);

    py::class_<ReferenceParams, TextDocumentPositionParams>(m, "ReferenceParams")
            .def(py::init<const TextDocumentIdentifier&, const Position&, bool>());
    
    py::class_<RenameParams, TextDocumentPositionParams>(m, "RenameParams")
            .def(py::init<const TextDocumentIdentifier&, const Position&, std::string>());

    py::class_<WorkspaceEdit>(m, "WorkspaceEdit")
            .def(py::init<>())
            .def("add_change", &WorkspaceEdit::add_change)
            // The changes map can be accessed directly from Python as a dictionary.
            .def_readwrite("changes", &WorkspaceEdit::changes);
    
    py::enum_<DiagnosticSeverity>(m, "DiagnosticSeverity")
            .value("Error", DiagnosticSeverity::Error)
            .value("Warning", DiagnosticSeverity::Warning)
            .value("Information", DiagnosticSeverity::Information)
            .value("Hint", DiagnosticSeverity::Hint)
            .export_values();

    py::class_<Diagnostic>(m, "Diagnostic")
            .def(py::init<Range, std::string, std::string, DiagnosticSeverity>())
            .def_readwrite("range", &Diagnostic::range)
            .def_readwrite("message", &Diagnostic::message)
            .def_readwrite("source", &Diagnostic::source)
            .def_readwrite("severity", &Diagnostic::severity);
    
    py::class_<FoldingRange>(m, "FoldingRange")
            .def_readwrite("start_line", &FoldingRange::startLine)
            .def_readwrite("start_character", &FoldingRange::startCharacter)
            .def_readwrite("end_line", &FoldingRange::endLine)
            .def_readwrite("end_character", &FoldingRange::endCharacter)
            .def_readwrite("kind", &FoldingRange::foldingRangeKind);

}

