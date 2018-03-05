#include "data_exceptions.h"

namespace navitia { namespace data {

wrong_version::~wrong_version() noexcept {}
data_loading_error::~data_loading_error() noexcept {}
disruptions_broken_connection::~disruptions_broken_connection() noexcept {}
disruptions_loading_error::~disruptions_loading_error() noexcept {}
raptor_building_error::~raptor_building_error() noexcept {}

}} //namespace navitia::type
