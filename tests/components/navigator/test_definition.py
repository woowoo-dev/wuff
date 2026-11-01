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
