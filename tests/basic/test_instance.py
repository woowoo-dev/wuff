def test_instantiation(analyzer):
    assert analyzer is not None


def test_dialect_with_bibliography_target_loads(analyzer, file1_uri):
    # The session dialect gives `cite` a reference with `target: bibliography` and no `meta_key`.
    # Loading it must not throw, and references to labels keep working.
    from wuff import CompletionParams, CompletionContext, CompletionTriggerKind, TextDocumentIdentifier, Position
    results = analyzer.complete(CompletionParams(
        TextDocumentIdentifier(file1_uri), Position(4, 41),
        CompletionContext(CompletionTriggerKind.TriggerCharacter, ":")))
    assert any(result.label == "ct1" for result in results)
