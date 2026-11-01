from wuff import TextDocumentIdentifier, Diagnostic, DiagnosticSeverity

def diagnose_document(analyzer, uri):
    return analyzer.diagnose(TextDocumentIdentifier(uri))

def test_diagnose_healthy(analyzer, file1_uri):
    diagnostics = diagnose_document(analyzer, file1_uri)
    assert len(diagnostics) == 0, "Expected no diagnostics for a healthy file"

def test_diagnose_errors(analyzer, empty_uri):
    diagnostics = diagnose_document(analyzer, empty_uri)
    assert len(diagnostics) == 1, "Expected exactly 1 diagnostic for an empty file"
    assert diagnostics[0].severity == DiagnosticSeverity.Error, "Expected the diagnostic severity to be 'Error'"

def test_diagnose_errors_missing(analyzer, file3_uri):
    diagnostics = diagnose_document(analyzer, file3_uri)
    assert len(diagnostics) == 1, "Expected exactly 1 diagnostic for file with missing elements"
    assert diagnostics[0].severity == DiagnosticSeverity.Error, "Expected the diagnostic severity to be 'Error'"
    assert diagnostics[0].message == "Syntax error: MISSING short_inner_environment_body", "Expected specific syntax error message"


def test_diagnose_upstream_dialect_entries(analyzer, upstream_uri):
    # Every structure that the dialect adopts from the fit-pdf/fit-html templates has to be valid
    # WooWoo as well: .Concept:, .Video: with .youtube: (playlist, aspect), .geogebra:, !dot:,
    # !pseudocode: (linenumber), .item: with bullet:, and the generic skip:/only: keys on
    # environments.
    diagnostics = diagnose_document(analyzer, upstream_uri)
    assert len(diagnostics) == 0, \
        f"Expected no diagnostics, got {[d.message for d in diagnostics]}"


def warnings_of(diagnostics):
    return [d for d in diagnostics if d.severity == DiagnosticSeverity.Warning]


def test_unknown_cite_warning(analyzer, bibliography_uri):
    diagnostics = diagnose_document(analyzer, bibliography_uri)
    warnings = sorted((d.range.start.line, d.range.start.character, d.range.end.line, d.range.end.character,
                       d.message, d.source) for d in warnings_of(diagnostics))
    assert warnings == [
        (5, 6, 5, 9, "'Tre' is not in the bibliography (references.bib)", "wuff"),
        (6, 6, 6, 18, "'Unknown_2000' is not in the bibliography (references.bib)", "wuff"),
    ]
    # the empty `.cite:` keeps only its syntax error
    assert [d.message for d in diagnostics if d.severity == DiagnosticSeverity.Error] == \
        ["Syntax error: MISSING short_inner_environment_body"]


def test_no_warning_for_labels(make_workspace, tmp_path):
    # labels can be defined in other textbooks (the catalogue), so an unknown label is not flagged
    workspace = make_workspace({"Woofile": "builder:\n  bibtex: refs.bib\n",
                                          "refs.bib": "@book{Known_2000, title = {K}}\n",
                                          "a.woo": ".Chapter A\n  label: a\n\n\nSee .reference:does-not-exist.\n"})
    assert diagnose_document(workspace, (tmp_path / 'a.woo').as_uri()) == []


def test_no_warning_without_bibliography(make_workspace, tmp_path):
    document = ".Chapter A\n  label: a\n\n\nSee .cite:Anything_2000.\n"
    workspace = make_workspace({"project/Woofile": "builder:\n  build_dir: build\n",
                                          "project/a.woo": document, "loose/b.woo": document})
    assert diagnose_document(workspace, (tmp_path / 'project' / 'a.woo').as_uri()) == []
    assert diagnose_document(workspace, (tmp_path / 'loose' / 'b.woo').as_uri()) == []
