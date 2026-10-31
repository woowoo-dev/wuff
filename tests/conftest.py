import os
from pathlib import Path
import pytest
import wuff


@pytest.fixture(scope="session")
def analyzer():
    analyzer = wuff.WooWooAnalyzer()

    dialect_path = Path(__file__).parent.parent.resolve() / "tests" / "files" / "fit_math.yaml"

    analyzer.set_dialect(str(dialect_path))
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