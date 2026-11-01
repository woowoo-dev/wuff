from wuff import ReferenceParams, TextDocumentIdentifier, Position, CompletionItem

def create_reference_params(uri, line, char, include_declaration):
    return ReferenceParams(TextDocumentIdentifier(uri), Position(line, char), include_declaration)

def get_references(analyzer, uri, line, char, include_declaration):
    params = create_reference_params(uri, line, char, include_declaration)
    return analyzer.references(params)

def test_find_references(analyzer, file2_uri):
    references = get_references(analyzer, file2_uri, 1, 11, False)
    assert len(references) == 3, "Expected to find 3 references"

def test_find_references_include_declaration(analyzer, file2_uri):
    references = get_references(analyzer, file2_uri, 1, 11, True)
    assert len(references) == 4, "Expected to find 4 references including the declaration"

# line 4 of bibliography.woo
CZECH_LINE = "My budeme čerpat zejména z učebnic .cite:Trefethen_Bau_1997 a .cite:Higham_2002."

def as_tuples(locations):
    return sorted((l.uri.rsplit("/", 1)[-1], l.range.start.line, l.range.start.character,
                   l.range.end.line, l.range.end.character) for l in locations)

def test_references_from_cite_usage(analyzer, bibliography_uri):
    start = CZECH_LINE.index("Trefethen_Bau_1997")
    usages = [("bibliography.woo", 4, start, 4, start + 18), ("bibliography.woo", 9, 12, 9, 30)]
    assert as_tuples(get_references(analyzer, bibliography_uri, 4, start + 2, False)) == usages
    assert as_tuples(get_references(analyzer, bibliography_uri, 4, start + 2, True)) == \
        sorted(usages + [("references.bib", 4, 6, 4, 24)])

def test_references_from_label_usage(analyzer, file1_uri, file2_uri):
    # from `.reference:ct2` the same as from its declaration `label: ct2`
    from_usage = as_tuples(get_references(analyzer, file1_uri, 4, 42, True))
    from_declaration = as_tuples(get_references(analyzer, file2_uri, 1, 11, True))
    assert from_usage == from_declaration
    assert len(from_usage) == 4
