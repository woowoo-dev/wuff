import os

from wuff import (
    CompletionContext, CompletionItemKind, CompletionParams, CompletionTriggerKind, Position,
    TextDocumentIdentifier, WooWooAnalyzer
)

# line 4 of bibliography.woo
CZECH_LINE = "My budeme čerpat zejména z učebnic .cite:Trefethen_Bau_1997 a .cite:Higham_2002."

# a document whose line 4 ends with `.cite:` (column 10) and line 5 with `.reference:` (column 15)
CITING_DOCUMENT = ".Chapter A\n  label: a\n\n\nSee .cite:\nSee .reference:\n"


def complete(analyzer, uri, line, char, trigger=":"):
    context = (CompletionContext(CompletionTriggerKind.TriggerCharacter, trigger) if trigger
               else CompletionContext(CompletionTriggerKind.Invoked, None))
    return analyzer.complete(CompletionParams(TextDocumentIdentifier(uri), Position(line, char), context))


def item(results, label):
    return next(result for result in results if result.label == label)


def make_workspace(root, files):
    for name, content in files.items():
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        if isinstance(content, bytes):
            path.write_bytes(content)
        else:
            path.write_text(content, encoding="utf-8")
    analyzer = WooWooAnalyzer()
    analyzer.set_completion_text_edits(True)
    analyzer.load_workspace(f"file://{root}")
    return analyzer


def cite_keys(analyzer, uri):
    return sorted(result.label for result in complete(analyzer, uri, 4, 10))


def test_cite_completion_lists_keys(analyzer, bibliography_uri):
    results = complete(analyzer, bibliography_uri, 8, 6)
    assert sorted(result.label for result in results) == ["Higham_2002", "Trefethen_Bau_1997", "Čech_1959"]
    assert all(result.kind == CompletionItemKind.Reference for result in results)
    assert item(results, "Trefethen_Bau_1997").detail == "Trefethen, Bau (1997): Numerical Linear Algebra"


def test_cite_completion_mid_key(analyzer, bibliography_uri):
    edit = item(complete(analyzer, bibliography_uri, 5, 9, trigger=None), "Trefethen_Bau_1997").text_edit
    r = edit.range
    assert (r.start.line, r.start.character, r.end.line, r.end.character) == (5, 6, 5, 9)
    assert edit.new_text == "Trefethen_Bau_1997"


def test_cite_completion_after_czech_text(analyzer, bibliography_uri):
    start = CZECH_LINE.index("Higham_2002")  # all characters are in the BMP: code points == UTF-16 units
    results = complete(analyzer, bibliography_uri, 4, start + 3, trigger=None)
    r = item(results, "Higham_2002").text_edit.range
    assert (r.start.character, r.end.character) == (start, start + len("Higham_2002"))


def test_bibliography_tolerates_junk(analyzer, tmp_path):
    bib = (
        '@comment{not an entry}\n'
        '@string{fit = "FIT"}\n'
        '@preamble{"\\newcommand{\\x}{y}"}\n'
        '@book{Good_2000, author = {A, B}, title = {T}, year = 2000}\n'
        'this is not bibtex at all\n'
        '@misc{NoFields}\n'
        '@article{Also_Good, title = "Quoted"}\n'
        '@book{Broken, author = {Unclosed\n'
    )
    workspace = make_workspace(tmp_path, {"Woofile": "builder:\n  bibtex: refs.bib\n", "refs.bib": bib,
                                          "a.woo": CITING_DOCUMENT})
    assert cite_keys(workspace, f"file://{tmp_path / 'a.woo'}") == ["Also_Good", "Good_2000", "NoFields"]


def test_bibtex_path_relative_to_woofile(analyzer, tmp_path):
    workspace = make_workspace(tmp_path, {"textbook/Woofile": "builder:\n  bibtex: ../shared/refs.bib\n",
                                          "shared/refs.bib": "@book{Shared_2020, title = {S}}\n",
                                          "textbook/a.woo": CITING_DOCUMENT})
    assert cite_keys(workspace, f"file://{tmp_path / 'textbook' / 'a.woo'}") == ["Shared_2020"]


def test_broken_woofile(analyzer, tmp_path, capfd):
    workspace = make_workspace(tmp_path, {"Woofile": ":::\n- [", "a.woo": CITING_DOCUMENT})
    uri = f"file://{tmp_path / 'a.woo'}"
    assert cite_keys(workspace, uri) == []
    assert [result.label for result in complete(workspace, uri, 5, 15)] == ["a"]
    # reported on stderr (the editor's output panel), not in a file in the user's folder
    assert "Could not read" in capfd.readouterr().err
    assert not (tmp_path / "log.txt").exists()


def test_woofile_without_bibtex_is_fine(analyzer, tmp_path, capfd):
    workspace = make_workspace(tmp_path, {"Woofile": "root_file: a.woo\nbuilder:\n  build_dir: build\n",
                                          "a.woo": CITING_DOCUMENT})
    assert cite_keys(workspace, f"file://{tmp_path / 'a.woo'}") == []
    assert "Could not read" not in capfd.readouterr().err


def test_missing_bib_file(analyzer, tmp_path):
    workspace = make_workspace(tmp_path, {"Woofile": "builder:\n  bibtex: missing.bib\n", "a.woo": CITING_DOCUMENT})
    assert cite_keys(workspace, f"file://{tmp_path / 'a.woo'}") == []


def touch_later(path):
    later = os.stat(path).st_mtime + 10
    os.utime(path, (later, later))


def test_bibliography_reloads(analyzer, tmp_path):
    workspace = make_workspace(tmp_path, {"Woofile": "builder:\n  bibtex: refs.bib\n",
                                          "refs.bib": "@book{First_2000, title = {F}}\n", "a.woo": CITING_DOCUMENT})
    uri = f"file://{tmp_path / 'a.woo'}"
    assert cite_keys(workspace, uri) == ["First_2000"]

    with open(tmp_path / "refs.bib", "a", encoding="utf-8") as bib:
        bib.write("@book{Second_2001, title = {S}}\n")
    touch_later(tmp_path / "refs.bib")
    assert cite_keys(workspace, uri) == ["First_2000", "Second_2001"]


def test_woofile_reloads(analyzer, tmp_path):
    workspace = make_workspace(tmp_path, {"Woofile": "", "refs.bib": "@book{First_2000, title = {F}}\n",
                                          "a.woo": CITING_DOCUMENT})
    uri = f"file://{tmp_path / 'a.woo'}"
    assert cite_keys(workspace, uri) == []

    (tmp_path / "Woofile").write_text("builder:\n  bibtex: refs.bib\n")
    touch_later(tmp_path / "Woofile")
    assert cite_keys(workspace, uri) == ["First_2000"]


def test_cite_completion_without_project(analyzer, tmp_path):
    workspace = make_workspace(tmp_path, {"a.woo": CITING_DOCUMENT})
    assert cite_keys(workspace, f"file://{tmp_path / 'a.woo'}") == []


def test_bibliography_not_utf8(analyzer, tmp_path):
    # an older .bib in ISO-8859-2: the invalid bytes become U+FFFD instead of breaking the completion
    bib = "@book{Cech_1959, author = {Čech, Eduard}, title = {Bodové množiny}, year = 1959}\n".encode("iso-8859-2")
    workspace = make_workspace(tmp_path, {"Woofile": "builder:\n  bibtex: refs.bib\n", "refs.bib": bib,
                                          "a.woo": CITING_DOCUMENT})
    results = complete(workspace, f"file://{tmp_path / 'a.woo'}", 4, 10)
    assert [result.label for result in results] == ["Cech_1959"]
    assert results[0].detail == "\ufffdech (1959): Bodov\ufffd mno\ufffdiny"


def test_no_warning_for_unreadable_bibliography(analyzer, tmp_path):
    # a .bib in UTF-16 (or a folder by mistake) yields no entries: treated as no bibliography, not as an empty one
    document = ".Chapter A\n  label: a\n\n\nSee .cite:Anything_2000.\n"
    utf16 = "@book{Anything_2000, title = {A}}\n".encode("utf-16")
    workspace = make_workspace(tmp_path, {"utf16/Woofile": "builder:\n  bibtex: refs.bib\n", "utf16/refs.bib": utf16,
                                          "utf16/a.woo": document,
                                          "folder/Woofile": "builder:\n  bibtex: refs\n", "folder/refs/x.txt": "",
                                          "folder/a.woo": document})
    for project in ("utf16", "folder"):
        diagnostics = workspace.diagnose(TextDocumentIdentifier(f"file://{tmp_path / project / 'a.woo'}"))
        assert [d.message for d in diagnostics] == [], project
