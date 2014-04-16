from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *

@dataset(["main_routing_test"])
class TestJourneys(AbstractTestFixture):

    def test_bidon2(self):
        logging.info("===========================================heho !")
        response = check_and_get_as_dict(self.tester, "/v1/coverage/main_routing_test")

