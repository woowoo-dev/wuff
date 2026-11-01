from wuff import Position, RenameParams, TextDocumentIdentifier, TextDocumentPositionParams

# line 4 of bibliography.woo
CZECH_LINE = "My budeme čerpat zejména z učebnic .cite:Trefethen_Bau_1997 a .cite:Higham_2002."


def prepare_rename(analyzer, uri, line, char):
    return analyzer.prepare_rename(TextDocumentPositionParams(TextDocumentIdentifier(uri), Position(line, char)))


def rename(analyzer, uri, line, char, new_name):
    return analyzer.rename(RenameParams(TextDocumentIdentifier(uri), Position(line, char), new_name))


def as_tuple(r):
    return (r.start.line, r.start.character, r.end.line, r.end.character)


def test_prepare_rename_label(analyzer, file1_uri, file2_uri):
    assert as_tuple(prepare_rename(analyzer, file2_uri, 1, 11)) == (1, 9, 1, 12)  # label: ct2
    assert as_tuple(prepare_rename(analyzer, file1_uri, 4, 42)) == (4, 41, 4, 44)  # .reference:ct2


def test_prepare_rename_nothing_to_rename(analyzer, file1_uri):
    assert prepare_rename(analyzer, file1_uri, 5, 2) is None  # "Thanks."


def test_prepare_rename_cite_refused(analyzer, bibliography_uri):
    assert prepare_rename(analyzer, bibliography_uri, 4, CZECH_LINE.index("Trefethen") + 2) is None


def test_rename_cite_is_empty(analyzer, bibliography_uri):
    edit = rename(analyzer, bibliography_uri, 4, CZECH_LINE.index("Trefethen") + 2, "Other_2000")
    assert edit.changes == {}


def test_rename_from_label_usage(analyzer, file1_uri):
    edit = rename(analyzer, file1_uri, 4, 42, "ct-two")
    edits = sorted((uri.rsplit("/", 1)[-1], as_tuple(e.range), e.new_text) for uri, es in edit.changes.items() for e in es)
    assert edits == [
        ("file1.woo", (4, 41, 4, 44), "ct-two"),
        ("file2.woo", (1, 9, 1, 12), "ct-two"),
        ("file2.woo", (8, 47, 8, 50), "ct-two"),
        ("file3.woo", (4, 25, 4, 28), "ct-two"),
    ]
