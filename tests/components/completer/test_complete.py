from wuff import (
    CompletionParams, Position, CompletionContext,
    CompletionTriggerKind, TextDocumentPositionParams,
    TextDocumentIdentifier, CompletionItem
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
