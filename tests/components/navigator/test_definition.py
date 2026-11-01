from wuff import TextDocumentIdentifier, Position, DefinitionParams

def create_definition_params(uri, line, char):
    return DefinitionParams(TextDocumentIdentifier(uri), Position(line, char))

def get_definition(analyzer, uri, line, char):
    params = create_definition_params(uri, line, char)
    return analyzer.go_to_definition(params)

def test_navigate_to_file(analyzer, file1_uri):
    definition = get_definition(analyzer, file1_uri, 8, 13)
    assert "empty.woo" in definition.uri, "Expected 'empty.woo' in the definition URI"

def test_navigate_to_definition(analyzer, file1_uri):
    definition = get_definition(analyzer, file1_uri, 4, 43)
    assert "file2.woo" in definition.uri, "Expected 'file2.woo' in the definition URI"
    assert definition.range.start.line == 1, "Expected start line to be 1"
    assert definition.range.start.character == 9, "Expected start character to be 9"

def test_definition_eqref_align_label(analyzer, eqref_uri):
    definition = get_definition(analyzer, eqref_uri, 22, 12)
    assert definition.uri.endswith("eqref.woo")
    assert (definition.range.start.line, definition.range.start.character) == (11, 9)

def test_definition_hash_gather_label(analyzer, eqref_uri):
    definition = get_definition(analyzer, eqref_uri, 22, 28)
    assert definition.uri.endswith("eqref.woo")
    assert (definition.range.start.line, definition.range.start.character) == (17, 9)

# line 4 of bibliography.woo
CZECH_LINE = "My budeme čerpat zejména z učebnic .cite:Trefethen_Bau_1997 a .cite:Higham_2002."

def assert_range(location, start_line, start_char, end_line, end_char):
    r = location.range
    assert (r.start.line, r.start.character, r.end.line, r.end.character) == (start_line, start_char, end_line, end_char)

def test_definition_cite(analyzer, bibliography_uri):
    definition = get_definition(analyzer, bibliography_uri, 4, CZECH_LINE.index("Trefethen") + 2)
    assert definition.uri.endswith("references.bib")
    assert_range(definition, 4, 6, 4, 24)

def test_definition_cite_czech_key(analyzer, bibliography_uri):
    # `Čech_1959` is 9 UTF-16 units but 10 UTF-8 bytes
    definition = get_definition(analyzer, bibliography_uri, 7, 8)
    assert definition.uri.endswith("references.bib")
    assert_range(definition, 19, 6, 19, 15)

def test_definition_cite_at_end_of_key(analyzer, bibliography_uri):
    end = CZECH_LINE.index("Higham_2002") + len("Higham_2002")
    definition = get_definition(analyzer, bibliography_uri, 4, end)
    assert_range(definition, 12, 6, 12, 17)

def test_definition_at_end_of_label(analyzer, file1_uri):
    # the cursor right after `.reference:ct2`
    definition = get_definition(analyzer, file1_uri, 4, 44)
    assert definition.uri.endswith("file2.woo")
    assert_range(definition, 1, 9, 1, 12)

def test_definition_cite_unknown_key(analyzer, bibliography_uri):
    assert get_definition(analyzer, bibliography_uri, 6, 8).uri == ""
