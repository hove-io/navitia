from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *


def is_valid_journey_response(response, tester):
    journeys = get_not_null(response, "journeys")

    all_sections = unique_dict('id')
    assert len(journeys) > 0, "we must at least have one journey"
    for j in journeys:
        is_valid_journey(j, tester)

        for s in j['sections']:
            all_sections[s['id']] = s

    # check the fare section
    # the fares must be structurally valid and all link to sections must be ok
    all_tickets = unique_dict('id')
    fares = get_not_null(response, "tickets")
    for f in fares:
        is_valid_ticket(f, tester)
        all_tickets[f['id']] = f

    check_internal_links(response, tester)


    #TODO check journey links (prev/next)


def is_valid_journey(journey, tester):
    #TODO!
    pass


def is_valid_ticket(ticket, tester):
    found = get_not_null(ticket, 'found')
    assert is_valid_bool(found)

    get_not_null(ticket, 'id')
    get_not_null(ticket, 'name')
    cost = get_not_null(ticket, 'cost')
    if found:
        #for found ticket, we must have a non empty currency
        get_not_null(cost, 'currency')

    assert is_valid_float(get_not_null(cost, 'value'))

    check_links(ticket, tester)


#default journey query used in varius test
journey_basic_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
    .format(from_coord="0.0000898312;0.0000898312",  # coordinate of S in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate of R in the dataset
            datetime="20120614T080000")


@dataset(["main_routing_test"])
class TestJourneys(AbstractTestFixture):
    """
    Test the structure of the journeys response
    """

    def test_journeys(self):
        #NOTE: we query /v1/coverage/main_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        response = self.query_region(journey_basic_query, display=False)

        is_valid_journey_response(response, self.tester)


@dataset([])
class TestJourneysNoRegion(AbstractTestFixture):
    """
    If no region loaded we must have a polite error while asking for a journey
    """

    def test_with_region(self):
        response, status = self.query_no_assert("v1/coverage/non_existent_region/" + journey_basic_query)

        assert status != 200, "the response should not be valid"

        assert response['error']['id'] == "unknown_object"
        assert response['error']['message'] == "The region non_existent_region doesn't exists"

    def test_no_region(self):
        response, status = self.query_no_assert("v1/" + journey_basic_query)

        assert status != 200, "the response should not be valid"

        assert response['error']['id'] == "unknown_object"

        error_regexp = re.compile('^No region available for the coordinates.*')
        assert error_regexp.match(response['error']['message'])


