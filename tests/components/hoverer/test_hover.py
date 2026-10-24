from wuff import TextDocumentPositionParams
from wuff import TextDocumentIdentifier
from wuff import Position


def test_hover_docpart(analyzer, file1_uri):
    result = analyzer.hover(TextDocumentPositionParams(
        TextDocumentIdentifier(file1_uri),
        Position(0, 3)))
    assert len(result) > 0, "Expected hover result to be non-empty"
    assert "document part" in result, "Expected 'document part' to be in the hover result"


def test_hover_none(analyzer, file1_uri):
    result = analyzer.hover(TextDocumentPositionParams(
        TextDocumentIdentifier(file1_uri),
        Position(2, 3)))
    assert len(result) == 0, "Expected hover result to be empty"


def test_hover_upstream_dialect_entries(analyzer, upstream_uri):
    # The dialect mirrors the fit-pdf/fit-html templates. Hovering the type of an entry that was
    # added in the sync returns its description from the dialect, i.e. the entry is really known.
    def hover(line, char):
        return analyzer.hover(TextDocumentPositionParams(
            TextDocumentIdentifier(upstream_uri), Position(line, char)))

    assert "informal definition of a concept" in hover(4, 3), "Expected the Concept description"
    assert "embedded videos" in hover(13, 3), "Expected the Video description"
    assert "YouTube video embed" in hover(17, 5), "Expected the youtube description"
    assert "GeoGebra material" in hover(31, 5), "Expected the geogebra description"
    assert "dot (Graphviz)" in hover(40, 2), "Expected the dot description"
    assert "pseudocode" in hover(51, 5), "Expected the pseudocode description"
    assert "description of a procedure" in hover(76, 3), "Expected the Procedure description"


def test_hover_lists_meta_block_fields(analyzer, upstream_uri):
    # The hover is Markdown: the description is followed by the meta-block fields of the entry,
    # generated from the dialect, so it cannot get out of sync with what the entry accepts.
    def hover(line, char):
        return analyzer.hover(TextDocumentPositionParams(
            TextDocumentIdentifier(upstream_uri), Position(line, char)))

    concept = hover(4, 3)
    assert "**Meta block fields**" in concept, "Expected the field list in the hover"
    assert "`nonumber`" in concept, "Expected the nonumber field of the wobjects"

    youtube = hover(17, 5)
    assert "`playlist`" in youtube and "`aspect`" in youtube, "Expected the youtube fields"
    assert "`16x9`" in youtube, "Expected the default aspect ratio to be documented"

    dot_hover = hover(40, 2)
    assert "`filename` **(required)**" in dot_hover, "Expected the required field to be marked"
    assert "fragile form: `!dot`" in dot_hover, "Expected the fragile form hint"
