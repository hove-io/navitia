NTFS - Fare extension
======================================

# Introduction
This document describes an extension of the core [NTFS (fr)](./ntfs_fr.md) data format to add fares and tickets information into Navitia.
The [old fare extension](./ntfs_fare_extension_fr_deprecated.md) is still in use in conjunction with the files described here for backward compatibility matters.

# Data format
The data format of the files are the same as the core [NTFS ones](./ntfs_fr.md). In short, files are CSV encoded in utf8 and with a comma `,` separator.

# Extension description
## Added files of this extension

File | Constraint | Comment
--- | --- | ---
tickets.txt | Required | Contains tickets declaration.
ticket_prices.txt | Required | Contains prices of a ticket.
ticket_uses.txt | Required | Contains where the ticket can be used
ticket_use_perimeters.txt | Required | Defines the networks and lines of a ticket_use
ticket_use_restrictions.txt | Optional | Defines a use of a ticket between zones or stop_areas

## Other modifications
The `object_properties.txt` and `object_codes.txt` files are extended to enable the reference to a ticket_id.

# Added files descriptions

## tickets.txt (Required)
Field | Type | Constraint | Description
--- | --- | --- | ---
ticket_id | Text | Required | Unique ID of the ticket
ticket_name | Text | Required | Name of the ticket (may be displayed to travelers)
ticket_comment | Text | Optional | Comment on the ticket that may be displayed to traveler

## ticket_prices.txt (Required)  
A ticket may have no price, either:
- no ticket_price is provided for the ticket,
- one or more ticket_prices are provided, but do not cover the validity period of the dataset.

Field | Type | Constraint | Description
--- | --- | --- | ---
ticket_id | Text | Required | Unique ID of the ticket
ticket_price | Non-negative Float | Required |
ticket_currency | Currency Code | Required | An ISO 4217 alphabetical currency code. For the list of current currency, refer to https://en.wikipedia.org/wiki/ISO_4217#Active_codes.
ticket_validity_start | Date | Required | First day of usability of the ticket (included)
ticket_validity_end | Date | Required | Last day of usability of the ticket (included)

## ticket_uses.txt (Required)
To be valid, a ticket must be associated to at least one ticket_use to describe the constraints of the ticket within the associated ticket_use_perimeter.

Field | Type | Constraint | Description
--- | --- | --- | ---
ticket_id | Text | Required | Unique ID of the ticket
ticket_use_id | Text | Required | Unique ID of the ticket use
max_transfers | integer | Required | Max number of transfers using one ticket. The value can be empty to indicate there is no constraint, but the field must be provided.
boarding_time_limit | integer | Required | Specifies the time frame (in seconds) in which a traveler can board in a new vehicle. The value can be empty to indicate there is no constraint, but the field must be provided.
alighting_time_limit | integer | Required | Specifies the time frame (in seconds) in which a traveler must get off the vehicle. When this duration is expired, the ticket is no longer valid. The value can be empty to indicate there is no constraint, but the field must be provided.

## ticket_use_perimeters.txt (Required)

A ticket_use must contain at least one included line or network.

Field | Type | Constraint | Description
--- | --- | --- | ---
ticket_use_id | Text | Required | Unique ID of the ticket use
object_type | Text | Required | This field can be either `line` or `network`
object_id | Text | Required | This field refers to the ID of the considered line or network.
perimeter_action | integer | Required | Value 1 for inclusion (for a `network` or a `line`) and 2 for an exclusion (for a `line` only)

## ticket_use_restrictions.txt (Optional)
Field | Type | Constraint | Description
--- | --- | --- | ---
ticket_use_id | Text | Required | Unique ID of the ticket use
restriction_type | Text | Required | either `zone` to specify a restriction based on `fare_zone_id` of stops or `OD` for a stop_area id use.
use_origin | Text | Required | Specification of the starting fare zone or stop_area of the ticket in use.
use_destination | Text | Required | Specification of the ending fare zone or stop_area of the ticket in use.
