import os
from pathlib import Path
import pytest
import wuff


@pytest.fixture(scope="session")
def analyzer():
    analyzer = wuff.WooWooAnalyzer()

    dialect_path = Path(__file__).parent.parent.resolve() / "tests" / "files" / "fit_math.yaml"

    analyzer.set_dialect(str(dialect_path))
    # like the language server: it applies the text edits of completion items
    analyzer.set_completion_text_edits(True)
    test_project_path = Path(__file__).parent.resolve() / "files" / "test_project"
    test_project_uri = f"file:///{test_project_path}".replace(os.sep, '/')
    analyzer.load_workspace(test_project_uri)
    yield analyzer


def generate_woo_file_uri(filename):
    test_project_path = Path(__file__).parent.resolve() / "files" / "test_project"
    file_path = test_project_path / filename
    file_uri = f"file:///{file_path}".replace(os.sep, '/')
    return file_uri


@pytest.fixture(scope="session")
def file1_uri():
    return generate_woo_file_uri('file1.woo')


@pytest.fixture(scope="session")
def file2_uri():
    return generate_woo_file_uri('file2.woo')


@pytest.fixture(scope="session")
def file3_uri():
    return generate_woo_file_uri('file3.woo')


@pytest.fixture(scope="session")
def nested_uri():
    return generate_woo_file_uri('nested.woo')


@pytest.fixture(scope="session")
def mathenv_uri():
    return generate_woo_file_uri('mathenv.woo')


@pytest.fixture(scope="session")
def upstream_uri():
    return generate_woo_file_uri('upstream.woo')


@pytest.fixture(scope="session")
def empty_uri():
    return generate_woo_file_uri('empty.woo')

@pytest.fixture(scope="session")
def eqref_uri():
    return generate_woo_file_uri('eqref.woo')


@pytest.fixture(scope="session")
def usages_uri():
    return generate_woo_file_uri('usages.woo')


@pytest.fixture(scope="session")
def bibliography_uri():
    return generate_woo_file_uri('bibliography.woo')


@pytest.fixture
def make_workspace(analyzer, tmp_path):
    """Returns a function that writes files (text, or bytes as they are) into tmp_path and gives a
    WooWooAnalyzer with that folder as its workspace. The dialect is the session one."""
    def make(files):
        for name, content in files.items():
            path = tmp_path / name
            path.parent.mkdir(parents=True, exist_ok=True)
            if isinstance(content, bytes):
                path.write_bytes(content)
            else:
                path.write_text(content, encoding="utf-8")
        workspace = wuff.WooWooAnalyzer()
        workspace.set_completion_text_edits(True)
        workspace.load_workspace(tmp_path.as_uri())
        return workspace
    return make
