from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *

@dataset([])
class TestEmptyEndPoint(AbstractTestFixture):
    """
    Test main entry points.
    Do not need running kraken
    """

    def test_index(self):
        json_response = check_and_get_as_dict(self.tester, "/")
        #TODO!

    def test_coverage(self):
        json_response = check_and_get_as_dict(self.tester, "/v1/coverage")
        #TODO!
