#include "utils/init.h"
#include "routing/tests/routing_api_test_data.h"
#include "mock_kraken.h"

int main() {
    navitia::init_app();

    routing_api_data<normal_speed_provider> routing_data;

    mock_kraken kraken(routing_data.b.data.release(), "main_routing_test");


    return 0;
}
