from wuff import TextDocumentIdentifier


def _token_positions(tokens):
    """Decode the delta-encoded LSP array into (line, start, length) triples (0-based)."""
    line = 0
    start = 0
    positions = []
    for i in range(0, len(tokens), 5):
        delta_line, delta_start, length = tokens[i], tokens[i + 1], tokens[i + 2]
        if delta_line:
            line += delta_line
            start = delta_start
        else:
            start += delta_start
        positions.append((line, start, length))
    return positions


def test_semantic_tokens(analyzer, file1_uri):
    tokens = analyzer.semantic_tokens(TextDocumentIdentifier(file1_uri))
    expected_token_count = 5  # Expected number of semantic tokens
    integers_per_token = 5  # According to LSP, each token is represented by 5 integers

    # Asserting the total number of integers (tokens * integers_per_token)
    total_integers = expected_token_count * integers_per_token
    assert len(
        tokens) == total_integers, f"Expected {total_integers} integers representing {expected_token_count} tokens"


def test_semantic_tokens_implicit_math(analyzer, file2_uri):
    # file2.woo contains a document-level indented block, which the fit_math dialect
    # parses as an implicit (anonymous) `equation` environment (`math: true`).
    # Three tokens come from the `label: ct2` meta block of the document part
    # (property, ':' operator, string), the remaining five are the LaTeX tokens of
    # the block `1 + 1 = 2` (number, operator, number, operator, number).
    tokens = analyzer.semantic_tokens(TextDocumentIdentifier(file2_uri))
    expected_token_count = 8
    integers_per_token = 5

    assert len(tokens) == expected_token_count * integers_per_token


def test_semantic_tokens_nested_implicit_math(analyzer, nested_uri):
    # An implicit environment is a formula not only at document level: inside a wobject body or an
    # item body it is indented one level deeper than the surrounding prose, and it must be
    # highlighted there as well. The bodies of those objects are ordinary `block`s (`.itemize:` and
    # `.item:` are outer environments), so the prose in them is never an implicit environment.
    positions = _token_positions(analyzer.semantic_tokens(TextDocumentIdentifier(nested_uri)))

    # lines with tokens: the meta blocks of the document part, of `.Definition:`, of `.itemize:` and
    # of both `.item:`s, plus the three formulas (`x^2 + y^2 = z^2`, `a = b`, `c = d`); the prose
    # lines have none
    assert sorted({line for line, _, _ in positions}) == [1, 5, 6, 10, 20, 28]
    assert len(positions) == 26


def test_semantic_tokens_math_environment(analyzer, mathenv_uri):
    # The body of a math environment (`.equation:`, `.align:`, `.gather:` — the environments with
    # `math: true` in the dialect) is coloured by the client's own `mathBlock` rule, so wuff must not
    # report anything there: neither the body lines themselves nor an indented block inside them
    # (`y = 2`, `e = f`), which is an implicit environment in the parse tree all the same. The body of
    # a fragile environment (`!pseudocode:`) is passed through verbatim and is not reported either.
    positions = _token_positions(analyzer.semantic_tokens(TextDocumentIdentifier(mathenv_uri)))

    # only the two meta blocks (of the document part and of `.equation:`); every formula line has none
    assert sorted({line for line, _, _ in positions}) == [1, 5]
    assert len(positions) == 6


def test_semantic_tokens_empty(analyzer, empty_uri):
    tokens = analyzer.semantic_tokens(TextDocumentIdentifier(empty_uri))
    assert len(tokens) == 0, "Expected no semantic tokens in an empty document"
