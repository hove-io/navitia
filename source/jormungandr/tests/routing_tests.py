from utils import *


@dataset(["main_ptref_test"])
class TestBidon(AbstractTestFixture):

    def test_bidon(self):
        logging.info("===========================================heho !")
        response = check_and_get_as_dict(self.tester, "/v1/coverage/main_ptref_test")


@dataset(["main_routing_test"])
class TestBidon2(AbstractTestFixture):

    def test_bidon2(self):
        logging.info("===========================================heho !")
        response = check_and_get_as_dict(self.tester, "/v1/coverage/main_routing_test")


@dataset([])
class TestBidon3(AbstractTestFixture):

    def test_bidon3(self):
        logging.info("===========================================heho !")
        response = check_and_get_as_dict(self.tester, "/v1/coverage")