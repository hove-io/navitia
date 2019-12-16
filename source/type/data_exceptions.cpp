#include "data_exceptions.h"

namespace navitia {
namespace data {

wrong_version::~wrong_version() noexcept = default;
data_loading_error::~data_loading_error() noexcept = default;
disruptions_broken_connection::~disruptions_broken_connection() noexcept = default;
disruptions_loading_error::~disruptions_loading_error() noexcept = default;
raptor_building_error::~raptor_building_error() noexcept = default;

}  // namespace data
}  // namespace navitia
