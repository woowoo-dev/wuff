from wuff import (
    CompletionParams, Position, CompletionContext,
    CompletionTriggerKind, TextDocumentPositionParams,
    TextDocumentIdentifier, CompletionItem, CompletionItemKind, WooWooAnalyzer
)

def create_completion_params(uri, line, char, trigger_char=":"):
    return CompletionParams(
        TextDocumentIdentifier(uri),
        Position(line, char),
        CompletionContext(CompletionTriggerKind.TriggerCharacter, trigger_char)
    )

def test_complete_reference(analyzer, file1_uri):
    cp = create_completion_params(file1_uri, 4, 41)
    results = analyzer.complete(cp)

    labels = ["ct1", "ct2", "ct3"]
    for label in labels:
        assert any(result.label == label for result in results), f"Label {label} not found in results"

def test_complete_nothing(analyzer, file1_uri):
    cp = create_completion_params(file1_uri, 0, 0)
    results = analyzer.complete(cp)
    assert len(results) == 0

def test_complete_eqref_align_label(analyzer, eqref_uri):
    # .eqref: targets the labels of all numbered equation environments: equation, align, gather
    cp = create_completion_params(eqref_uri, 22, 11)
    labels = {result.label for result in analyzer.complete(cp)}
    assert {"eq-two", "al-one", "ga-one"} <= labels, f"Got {labels}"

def create_invoked_params(uri, line, char):
    return CompletionParams(
        TextDocumentIdentifier(uri),
        Position(line, char),
        CompletionContext(CompletionTriggerKind.Invoked, None)
    )

def item_by_label(results, label):
    return next((result for result in results if result.label == label), None)

def test_complete_invoked_mid_label(analyzer, usages_uri):
    # Ctrl+Space at the end of `.reference:ct` offers the labels; the item replaces `ct`
    results = analyzer.complete(create_invoked_params(usages_uri, 4, 17))
    item = item_by_label(results, "ct1")
    assert item is not None, f"Got {[r.label for r in results]}"
    assert item.kind == CompletionItemKind.Reference
    assert item.text_edit.new_text == "ct1"
    r = item.text_edit.range
    assert (r.start.line, r.start.character, r.end.line, r.end.character) == (4, 15, 4, 17)

def test_complete_label_with_dash(analyzer, usages_uri):
    # `-` is not a word character in VS Code; the edit range still covers the whole `sec-`
    results = analyzer.complete(create_invoked_params(usages_uri, 5, 19))
    item = item_by_label(results, "sec-two")
    assert item is not None, f"Got {[r.label for r in results]}"
    assert item.text_edit.range.start.character == 15
    assert item.text_edit.range.end.character == 19

def test_complete_after_czech_text(analyzer, usages_uri):
    # `Viz učebnice .reference:` is 24 UTF-16 units but 25 UTF-8 bytes long (č)
    results = analyzer.complete(create_completion_params(usages_uri, 6, 24))
    item = item_by_label(results, "ct1")
    assert item is not None, f"Got {[r.label for r in results]}"
    r = item.text_edit.range
    assert (r.start.line, r.start.character, r.end.line, r.end.character) == (6, 24, 6, 24)

def test_complete_duplicate_label_offered_once(analyzer, tmp_path):
    # the same label defined twice (an author's mistake) is offered once
    (tmp_path / "Woofile").write_text("")
    (tmp_path / "a.woo").write_text(".Chapter A\n  label: twice\n\n\nSee .reference:\n")
    (tmp_path / "b.woo").write_text(".Chapter B\n  label: twice\n")
    project = WooWooAnalyzer()
    project.load_workspace(f"file://{tmp_path}")
    results = project.complete(create_completion_params(f"file://{tmp_path / 'a.woo'}", 4, 15))
    assert [result.label for result in results].count("twice") == 1


def test_invoked_completion_needs_text_edits(analyzer, usages_uri):
    # a server that does not apply text edits (language server 1.2) keeps the completion of wuff 1.3:
    # nothing on Ctrl+Space inside a value, where its insertion would replace only part of `sec-`
    plain = WooWooAnalyzer()
    test_project = usages_uri.rsplit("/", 1)[0]
    plain.load_workspace(test_project)
    assert plain.complete(create_invoked_params(usages_uri, 5, 19)) == []
    assert any(result.label == "ct1" for result in plain.complete(create_completion_params(usages_uri, 6, 24)))
