from wuff import (
    DefinitionParams, Position, ReferenceParams, TextDocumentIdentifier
)

CHAPTER = ".Chapter A\n  label: cap-a\n\n\nSee .reference:cap-a.\n"


def test_file_name_with_capitals_is_one_document(make_workspace, tmp_path):
    # wuff lower-cases the path of a URI on macOS and Windows; a file with capitals in its name has to be
    # found anyway, and opening it must not load a second copy (whose old text would answer requests)
    workspace = make_workspace({"Woofile": "", "Chapter-A.woo": CHAPTER})
    document = TextDocumentIdentifier((tmp_path / "Chapter-A.woo").as_uri())
    workspace.open_document(document)

    references = workspace.references(ReferenceParams(document, Position(4, 17), False))
    assert [(r.range.start.line, r.range.start.character) for r in references] == [(4, 15)]


def test_include_of_missing_file(make_workspace, tmp_path):
    # Ctrl+click on `.include` of a file that does not exist finds nothing instead of failing
    workspace = make_workspace({"Woofile": "", "a.woo": ".Chapter A\n  label: a\n\n\n.include missing.woo\n"})
    document = TextDocumentIdentifier((tmp_path / "a.woo").as_uri())
    assert workspace.go_to_definition(DefinitionParams(document, Position(4, 12))).uri == ""
