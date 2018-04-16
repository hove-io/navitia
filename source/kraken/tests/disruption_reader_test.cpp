/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE disruption_reader_test
#include <boost/test/unit_test.hpp>
#include "kraken/fill_disruption_from_database.h"
#include <map>

struct Const_it {
    struct Value {
        std::string value = "";
        Value() {}
        Value(const std::string& value) : value(value) {}

        template<typename T>
        T as() {
            return boost::lexical_cast<T>(value);
        }


        bool is_null() {
            return value == "";
        }
    };

    std::map<std::string, Value> values;

    Const_it() { reinit();}

    Value operator[] (const std::string& key) {
        return values[key];
    }

    void reinit() {
        values = {
            {"disruption_id", Value()},
            {"disruption_created_at", Value()},
            {"disruption_updated_at", Value()},
            {"disruption_start_publication_date", Value()},
            {"disruption_end_publication_date", Value()},
            {"disruption_note", Value()},
            {"disruption_reference", Value()},
            {"cause_id", Value()},
            {"cause_wording", Value()},
            {"cause_created_at", Value()},
            {"cause_updated_at", Value()},
            {"tag_id", Value()},
            {"tag_name", Value()},
            {"tag_created_at", Value()},
            {"tag_updated_at", Value()},
            {"impact_id", Value()},
            {"impact_created_at", Value()},
            {"impact_updated_at", Value()},
            {"severity_id", Value()},
            {"severity_created_at", Value()},
            {"severity_updated_at", Value()},
            {"severity_effect", Value()},
            {"severity_wording", Value()},
            {"severity_color", Value()},
            {"severity_priority", Value()},
            {"application_id", Value()},
            {"application_start_date", Value()},
            {"application_end_date", Value()},
            {"ptobject_id", Value()},
            {"ptobject_created_at", Value()},
            {"ptobject_updated_at", Value()},
            {"ptobject_uri", Value()},
            {"ptobject_type", Value()},
            {"property_key", Value()},
            {"property_type", Value()},
            {"property_value", Value()},
            {"message_id", Value()},
            {"message_text", Value()},
            {"message_created_at", Value()},
            {"message_updated_at", Value()},
            {"channel_id", Value()},
            {"channel_name", Value()},
            {"channel_content_type", Value()},
            {"channel_max_size", Value()},
            {"channel_created_at", Value()},
            {"channel_updated_at", Value()},
            {"channel_type_id", Value()},
            {"channel_type", Value()}
        };
    }

    void set_disruption(const std::string& id,
                        const std::string& created_at,
                        const std::string& updated_at = "",
                        const std::string& start_publication_date = "",
                        const std::string& end_publication_date = "",
                        const std::string& note = "",
                        const std::string& reference = "") {
        values["disruption_id"] = Value(id);
        values["disruption_created_at"] = Value(created_at);
        values["disruption_updated_at"] = Value(updated_at);
        values["disruption_start_publication_date"] = Value(start_publication_date);
        values["disruption_end_publication_date"] = Value(end_publication_date);
        values["disruption_note"] = Value(note);
        values["disruption_reference"] = Value(reference);
    }

    void set_cause(const std::string& id,
                   const std::string& wording,
                   const std::string& created_at,
                   const std::string& updated_at = "") {
        values["cause_id"] = Value(id);
        values["cause_created_at"] = Value(created_at);
        values["cause_updated_at"] = Value(updated_at);
        values["cause_wording"] = Value(wording);
    }

    void set_tag(const std::string& id,
                 const std::string& name,
                 const std::string& created_at,
                 const std::string& updated_at = "") {
        values["tag_id"] = Value(id);
        values["tag_created_at"] = Value(created_at);
        values["tag_updated_at"] = Value(updated_at);
        values["tag_name"] = Value(name);
    }

    void set_impact(const std::string& id,
                 const std::string& created_at,
                 const std::string& updated_at = "") {
        values["impact_id"] = Value(id);
        values["impact_created_at"] = Value(created_at);
        values["impact_updated_at"] = Value(updated_at);
    }

    void set_severity(const std::string& id,
                 const std::string& wording,
                 const std::string& created_at,
                 const std::string& updated_at = "",
                 const std::string& effect = "",
                 const std::string& color = "",
                 const std::string& priority = "") {
        values["severity_id"] = Value(id);
        values["severity_created_at"] = Value(created_at);
        values["severity_updated_at"] = Value(updated_at);
        values["severity_effect"] = Value(effect);
        values["severity_wording"] = Value(wording);
        values["severity_color"] = Value(color);
        values["severity_priority"] = Value(priority);
    }

    void set_application_period(const std::string& id,
            const std::string& start,
            const std::string& end) {
        values["application_id"] = id;
        values["application_start_date"] = start;
        values["application_end_date"] = end;
    }

    void set_ptobject(
            const std::string& id,
            const std::string& uri,
            const std::string& type,
            const std::string& created_at,
            const std::string& updated_at = ""
            ) {

            values["ptobject_id"] = id;
            values["ptobject_created_at"] = created_at;
            values["ptobject_updated_at"] = updated_at;
            values["ptobject_uri"] = uri;
            values["ptobject_type"] = type;
    }

    void set_property(
            const std::string& key,
            const std::string& type,
            const std::string& value
            ) {

            values["property_key"] = key;
            values["property_type"] = type;
            values["property_value"] = value;
    }

    void set_message(const std::string& id,
                     const std::string& text,
                     const std::string& created_at,
                     const std::string& updated_at = "") {
        values["message_id"] = id;
        values["message_created_at"] = created_at;
        values["message_updated_at"] = updated_at;
        values["message_text"] = text;
    }

    void set_channel(const std::string& id,
                     const std::string& name,
                     const std::string& content_type,
                     const std::string& size,
                     const std::string& created_at,
                     const std::string& updated_at = "") {
        values["channel_id"] = id;
        values["channel_name"] = name;
        values["channel_content_type"] = content_type;
        values["channel_max_size"] = size;
        values["channel_created_at"] = created_at;
        values["channel_updated_at"] = updated_at;
    }

    void set_channel_type(const std::string& id,
                     const std::string& type) {
        values["channel_type_id"] = id;
        values["channel_type"] = type;
    }
};


BOOST_AUTO_TEST_CASE(minimal_disruption) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);

    Const_it const_it;
    const_it.set_disruption("1", "22");
    reader.fill_disruption(const_it);
    BOOST_CHECK_EQUAL(reader.disruption->id(), "1");
    BOOST_CHECK_EQUAL(reader.disruption->created_at(), 22);
    BOOST_CHECK_EQUAL(reader.disruption->updated_at(), 0);
    BOOST_CHECK_EQUAL(reader.disruption->publication_period().start(), 0);
    BOOST_CHECK_EQUAL(reader.disruption->publication_period().end(), 0);
    BOOST_CHECK_EQUAL(reader.disruption->note(), "");
    BOOST_CHECK_EQUAL(reader.disruption->reference(), "");
}



BOOST_AUTO_TEST_CASE(full_disruption) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);

    Const_it const_it;
    const_it.set_disruption("1", "22", "33", "44", "55", "note", "reference");
    reader.fill_disruption(const_it);
    BOOST_CHECK_EQUAL(reader.disruption->id(), "1");
    BOOST_CHECK_EQUAL(reader.disruption->created_at(), 22);
    BOOST_CHECK_EQUAL(reader.disruption->updated_at(), 33);
    BOOST_CHECK_EQUAL(reader.disruption->publication_period().start(), 44);
    BOOST_CHECK_EQUAL(reader.disruption->publication_period().end(), 55);
    BOOST_CHECK_EQUAL(reader.disruption->note(), "note");
    BOOST_CHECK_EQUAL(reader.disruption->reference(), "reference");
}

BOOST_AUTO_TEST_CASE(minimal_cause) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);
    reader.disruption = std::make_unique<chaos::Disruption>();

    Const_it const_it;
    const_it.set_cause("1", "wording", "11");
    reader.fill_cause(const_it);
    BOOST_REQUIRE(reader.disruption->mutable_cause() != nullptr);
    auto cause = reader.disruption->cause();
    BOOST_CHECK_EQUAL(cause.id(), "1");
    BOOST_CHECK_EQUAL(cause.wording(), "wording");
    BOOST_CHECK_EQUAL(cause.created_at(), 11);
}

BOOST_AUTO_TEST_CASE(full_cause) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);
    reader.disruption = std::make_unique<chaos::Disruption>();

    Const_it const_it;
    const_it.set_cause("1", "wording", "11", "22");
    reader.fill_cause(const_it);
    BOOST_REQUIRE(reader.disruption->mutable_cause() != nullptr);
    auto cause = reader.disruption->cause();
    BOOST_CHECK_EQUAL(cause.id(), "1");
    BOOST_CHECK_EQUAL(cause.wording(), "wording");
    BOOST_CHECK_EQUAL(cause.created_at(), 11);
    BOOST_CHECK_EQUAL(cause.updated_at(), 22);
}

BOOST_AUTO_TEST_CASE(minimal_tag) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);
    reader.disruption = std::make_unique<chaos::Disruption>();

    Const_it const_it;
    const_it.set_tag("1", "name", "11");
    reader.fill_tag(const_it);
    BOOST_REQUIRE_EQUAL(reader.disruption->tags_size(), 1);
    auto tag = reader.disruption->tags(0);
    BOOST_CHECK_EQUAL(tag.id(), "1");
    BOOST_CHECK_EQUAL(tag.name(), "name");
    BOOST_CHECK_EQUAL(tag.created_at(), 11);
}
BOOST_AUTO_TEST_CASE(full_tag) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);
    reader.disruption = std::make_unique<chaos::Disruption>();

    Const_it const_it;
    const_it.set_tag("1", "name", "11", "22");
    reader.fill_tag(const_it);
    BOOST_REQUIRE_EQUAL(reader.disruption->tags_size(), 1);
    auto tag = reader.disruption->tags(0);
    BOOST_CHECK_EQUAL(tag.id(), "1");
    BOOST_CHECK_EQUAL(tag.name(), "name");
    BOOST_CHECK_EQUAL(tag.created_at(), 11);
    BOOST_CHECK_EQUAL(tag.updated_at(), 22);
}

BOOST_AUTO_TEST_CASE(minimal_impact) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);
    reader.disruption = std::make_unique<chaos::Disruption>();

    Const_it const_it;
    const_it.set_impact("1", "11");
    const_it.set_severity("2", "wording", "22");
    reader.fill_impact(const_it);
    BOOST_REQUIRE_EQUAL(reader.disruption->impacts_size(), 1);
    auto impact = reader.disruption->impacts(0);
    BOOST_CHECK_EQUAL(impact.id(), "1");
    BOOST_CHECK_EQUAL(impact.created_at(), 11);
    BOOST_CHECK_EQUAL(impact.severity().id(), "2");
    BOOST_CHECK_EQUAL(impact.severity().created_at(), 22);
    BOOST_CHECK_EQUAL(impact.severity().wording(), "wording");
}

BOOST_AUTO_TEST_CASE(full_impact) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);
    reader.disruption = std::make_unique<chaos::Disruption>();

    Const_it const_it;
    const_it.set_impact("1", "11", "22");
    const_it.set_severity("2", "wording", "22", "33",
            "blocking", "color", "2");
    reader.fill_impact(const_it);
    BOOST_REQUIRE_EQUAL(reader.disruption->impacts_size(), 1);
    auto impact = reader.disruption->impacts(0);
    BOOST_CHECK_EQUAL(impact.id(), "1");
    BOOST_CHECK_EQUAL(impact.created_at(), 11);
    BOOST_CHECK_EQUAL(impact.updated_at(), 22);
    BOOST_CHECK_EQUAL(impact.severity().id(), "2");
    BOOST_CHECK_EQUAL(impact.severity().created_at(), 22);
    BOOST_CHECK_EQUAL(impact.severity().wording(), "wording");
    BOOST_CHECK_EQUAL(impact.severity().updated_at(), 33);
    BOOST_CHECK_EQUAL(impact.severity().color(), "color");
    BOOST_CHECK_EQUAL(impact.severity().effect(), transit_realtime::Alert::Effect::Alert_Effect_NO_SERVICE);
}

BOOST_AUTO_TEST_CASE(application_period) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);

    Const_it const_it;
    const_it.set_application_period("0", "1", "2");
    reader.impact = new chaos::Impact();
    reader.fill_application_period(const_it);
    BOOST_CHECK_EQUAL(reader.impact->application_periods_size(), 1);
    auto application = reader.impact->application_periods(0);
    BOOST_CHECK_EQUAL(application.start(), 1);
    BOOST_CHECK_EQUAL(application.end(), 2);
}

BOOST_AUTO_TEST_CASE(pt_object) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);
    reader.disruption = std::make_unique<chaos::Disruption>();

    Const_it const_it;
    const_it.set_ptobject("id", "uri", "line", "1", "2");
    reader.impact = new chaos::Impact();
    auto* ptobject = reader.impact->add_informed_entities();
    reader.fill_pt_object(const_it, ptobject);
    BOOST_REQUIRE_EQUAL(reader.impact->informed_entities_size(), 1);
    BOOST_CHECK_EQUAL(ptobject->uri(), "uri");
    BOOST_CHECK_EQUAL(ptobject->pt_object_type(), chaos::PtObject_Type_line);
    BOOST_CHECK_EQUAL(ptobject->created_at(), 1);
    BOOST_CHECK_EQUAL(ptobject->updated_at(), 2);
}

BOOST_AUTO_TEST_CASE(property) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);
    reader.disruption = std::make_unique<chaos::Disruption>();

    Const_it const_it;
    const_it.set_property("key", "type", "42");
    reader.fill_property(const_it);
    BOOST_REQUIRE_EQUAL(reader.disruption->properties_size(), 1);
    auto property = reader.disruption->properties(0);
    BOOST_CHECK_EQUAL(property.key(), "key");
    BOOST_CHECK_EQUAL(property.type(), "type");
    BOOST_CHECK_EQUAL(property.value(), "42");
}

BOOST_AUTO_TEST_CASE(one_of_each) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);

    Const_it const_it;
    const_it.set_disruption("1", "22", "33", "44", "55",
            "note", "reference");
    const_it.set_cause("1", "wording", "11", "22");
    const_it.set_tag("1", "name", "11", "22");
    const_it.set_impact("1", "11", "22");
    const_it.set_severity("2", "wording", "22", "33",
            "blocking", "color", "2");
    const_it.set_application_period("0", "1", "2");
    const_it.set_ptobject("id", "uri", "line", "1", "2");
    const_it.set_property("key", "type", "42");
    reader(const_it);
    const auto& disruption = reader.disruption;
    BOOST_CHECK_EQUAL(disruption->id(), "1");
    BOOST_CHECK_EQUAL(disruption->created_at(), 22);
    BOOST_CHECK_EQUAL(disruption->updated_at(), 33);
    BOOST_CHECK_EQUAL(disruption->publication_period().start(), 44);
    BOOST_CHECK_EQUAL(disruption->publication_period().end(), 55);
    BOOST_CHECK_EQUAL(disruption->note(), "note");
    BOOST_CHECK_EQUAL(disruption->reference(), "reference");
    auto cause = disruption->cause();
    BOOST_CHECK_EQUAL(cause.id(), "1");
    BOOST_CHECK_EQUAL(cause.wording(), "wording");
    BOOST_CHECK_EQUAL(cause.created_at(), 11);
    BOOST_CHECK_EQUAL(cause.updated_at(), 22);
    BOOST_REQUIRE_EQUAL(disruption->tags_size(), 1);
    auto tag = disruption->tags(0);
    BOOST_CHECK_EQUAL(tag.id(), "1");
    BOOST_CHECK_EQUAL(tag.name(), "name");
    BOOST_CHECK_EQUAL(tag.created_at(), 11);
    BOOST_CHECK_EQUAL(tag.updated_at(), 22);
    BOOST_REQUIRE_EQUAL(disruption->impacts_size(), 1);
    auto impact = disruption->impacts(0);
    BOOST_CHECK_EQUAL(impact.id(), "1");
    BOOST_CHECK_EQUAL(impact.created_at(), 11);
    BOOST_CHECK_EQUAL(impact.updated_at(), 22);
    auto severity = impact.severity();
    BOOST_CHECK_EQUAL(severity.id(), "2");
    BOOST_CHECK_EQUAL(severity.created_at(), 22);
    BOOST_CHECK_EQUAL(severity.wording(), "wording");
    BOOST_CHECK_EQUAL(severity.updated_at(), 33);
    BOOST_CHECK_EQUAL(severity.color(), "color");
    BOOST_CHECK_EQUAL(severity.effect(), transit_realtime::Alert::Effect::Alert_Effect_NO_SERVICE);
    BOOST_REQUIRE_EQUAL(impact.application_periods_size(), 1);
    auto application = impact.application_periods(0);
    BOOST_CHECK_EQUAL(application.start(), 1);
    BOOST_CHECK_EQUAL(application.end(), 2);
    BOOST_REQUIRE_EQUAL(impact.informed_entities_size(), 1);
    auto ptobject = impact.informed_entities(0);
    BOOST_CHECK_EQUAL(ptobject.uri(), "uri");
    BOOST_CHECK_EQUAL(ptobject.pt_object_type(), chaos::PtObject_Type_line);
    BOOST_CHECK_EQUAL(ptobject.created_at(), 1);
    BOOST_CHECK_EQUAL(ptobject.updated_at(), 2);
    auto property = disruption->properties(0);
    BOOST_CHECK_EQUAL(property.key(), "key");
    BOOST_CHECK_EQUAL(property.type(), "type");
    BOOST_CHECK_EQUAL(property.value(), "42");
}
BOOST_AUTO_TEST_CASE(two_tags) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);

    Const_it const_it;
    const_it.set_disruption("1", "22", "33", "44", "55",
            "note", "reference");
    const_it.set_cause("1", "wording", "11", "22");
    const_it.set_tag("1", "name", "11", "22");
    const_it.set_impact("1", "11", "22");
    const_it.set_severity("2", "wording", "22", "33",
            "blocking", "color", "2");
    const_it.set_application_period("0", "1", "2");
    const_it.set_ptobject("id", "uri", "line", "1", "2");
    reader(const_it);
    const_it.set_tag("2", "name2", "33", "44");
    reader(const_it);
    const auto& disruption = reader.disruption;
    BOOST_CHECK_EQUAL(disruption->id(), "1");
    auto cause = disruption->cause();
    BOOST_CHECK_EQUAL(cause.id(), "1");
    BOOST_REQUIRE_EQUAL(disruption->tags_size(), 2);
    auto tag = disruption->tags(0);
    BOOST_CHECK_EQUAL(tag.id(), "1");
    BOOST_CHECK_EQUAL(tag.name(), "name");
    BOOST_CHECK_EQUAL(tag.created_at(), 11);
    BOOST_CHECK_EQUAL(tag.updated_at(), 22);
    BOOST_REQUIRE_EQUAL(disruption->impacts_size(), 1);
    tag = disruption->tags(1);
    BOOST_CHECK_EQUAL(tag.id(), "2");
    BOOST_CHECK_EQUAL(tag.name(), "name2");
    BOOST_CHECK_EQUAL(tag.created_at(), 33);
    BOOST_CHECK_EQUAL(tag.updated_at(), 44);
    BOOST_REQUIRE_EQUAL(disruption->impacts_size(), 1);
    auto impact = disruption->impacts(0);
    BOOST_CHECK_EQUAL(impact.id(), "1");
    auto severity = impact.severity();
    BOOST_CHECK_EQUAL(severity.id(), "2");
    BOOST_REQUIRE_EQUAL(impact.application_periods_size(), 1);
    auto application = impact.application_periods(0);
    BOOST_CHECK_EQUAL(application.start(), 1);
    BOOST_REQUIRE_EQUAL(impact.informed_entities_size(), 1);
    auto ptobject = impact.informed_entities(0);
    BOOST_CHECK_EQUAL(ptobject.uri(), "uri");
}
BOOST_AUTO_TEST_CASE(two_impacts) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);

    Const_it const_it;
    const_it.set_disruption("1", "22", "33", "44", "55",
            "note", "reference");
    const_it.set_cause("1", "wording", "11", "22");
    const_it.set_tag("1", "name", "11", "22");
    const_it.set_impact("1", "11", "22");
    const_it.set_severity("2", "wording", "22", "33",
            "blocking", "color", "2");
    const_it.set_application_period("0", "1", "2");
    const_it.set_ptobject("id", "uri", "line", "1", "2");
    reader(const_it);
    const_it.set_impact("2", "33", "44");
    reader(const_it);
    const auto& disruption = reader.disruption;
    BOOST_CHECK_EQUAL(disruption->id(), "1");
    auto cause = disruption->cause();
    BOOST_CHECK_EQUAL(cause.id(), "1");
    BOOST_REQUIRE_EQUAL(disruption->tags_size(), 1);
    auto tag = disruption->tags(0);
    BOOST_CHECK_EQUAL(tag.id(), "1");
    BOOST_CHECK_EQUAL(tag.name(), "name");
    BOOST_CHECK_EQUAL(tag.created_at(), 11);
    BOOST_CHECK_EQUAL(tag.updated_at(), 22);
    BOOST_REQUIRE_EQUAL(disruption->impacts_size(), 2);
    auto impact = disruption->impacts(0);
    BOOST_CHECK_EQUAL(impact.id(), "1");
    BOOST_CHECK_EQUAL(impact.created_at(), 11);
    BOOST_CHECK_EQUAL(impact.updated_at(), 22);
    impact = disruption->impacts(1);
    BOOST_CHECK_EQUAL(impact.id(), "2");
    BOOST_CHECK_EQUAL(impact.created_at(), 33);
    BOOST_CHECK_EQUAL(impact.updated_at(), 44);
    auto severity = impact.severity();
    BOOST_CHECK_EQUAL(severity.id(), "2");
    BOOST_REQUIRE_EQUAL(impact.application_periods_size(), 1);
    auto application = impact.application_periods(0);
    BOOST_CHECK_EQUAL(application.start(), 1);
    BOOST_REQUIRE_EQUAL(impact.informed_entities_size(), 1);
    auto ptobject = impact.informed_entities(0);
    BOOST_CHECK_EQUAL(ptobject.uri(), "uri");
}

BOOST_AUTO_TEST_CASE(two_application_periods) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);

    Const_it const_it;
    const_it.set_disruption("1", "22", "33", "44", "55",
            "note", "reference");
    const_it.set_cause("1", "wording", "11", "22");
    const_it.set_tag("1", "name", "11", "22");
    const_it.set_impact("1", "11", "22");
    const_it.set_severity("2", "wording", "22", "33",
            "blocking", "color", "2");
    const_it.set_application_period("0", "1", "2");
    const_it.set_ptobject("id", "uri", "line", "1", "2");
    reader(const_it);
    const_it.set_application_period("1", "2", "3");
    reader(const_it);
    const auto& disruption = reader.disruption;
    BOOST_CHECK_EQUAL(disruption->id(), "1");
    auto cause = disruption->cause();
    BOOST_CHECK_EQUAL(cause.id(), "1");
    BOOST_REQUIRE_EQUAL(disruption->tags_size(), 1);
    auto tag = disruption->tags(0);
    BOOST_CHECK_EQUAL(tag.id(), "1");
    BOOST_CHECK_EQUAL(tag.name(), "name");
    BOOST_CHECK_EQUAL(tag.created_at(), 11);
    BOOST_CHECK_EQUAL(tag.updated_at(), 22);
    BOOST_REQUIRE_EQUAL(disruption->impacts_size(), 1);
    auto impact = disruption->impacts(0);
    BOOST_CHECK_EQUAL(impact.id(), "1");
    BOOST_CHECK_EQUAL(impact.created_at(), 11);
    BOOST_CHECK_EQUAL(impact.updated_at(), 22);
    auto severity = impact.severity();
    BOOST_CHECK_EQUAL(severity.id(), "2");
    BOOST_REQUIRE_EQUAL(impact.application_periods_size(), 2);
    auto application = impact.application_periods(0);
    BOOST_CHECK_EQUAL(application.start(), 1);
    application = impact.application_periods(1);
    BOOST_CHECK_EQUAL(application.start(), 2);
    BOOST_REQUIRE_EQUAL(impact.informed_entities_size(), 1);
    auto ptobject = impact.informed_entities(0);
    BOOST_CHECK_EQUAL(ptobject.uri(), "uri");
}

BOOST_AUTO_TEST_CASE(two_ptobjects) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);

    Const_it const_it;
    const_it.set_disruption("1", "22", "33", "44", "55",
            "note", "reference");
    const_it.set_cause("1", "wording", "11", "22");
    const_it.set_tag("1", "name", "11", "22");
    const_it.set_impact("1", "11", "22");
    const_it.set_severity("2", "wording", "22", "33",
            "blocking", "color", "2");
    const_it.set_application_period("0", "1", "2");
    const_it.set_ptobject("id", "uri", "line", "1", "2");
    reader(const_it);
    const_it.set_ptobject("id2", "uri2", "line", "1", "2");
    reader(const_it);
    const auto& disruption = reader.disruption;
    BOOST_CHECK_EQUAL(disruption->id(), "1");
    auto cause = disruption->cause();
    BOOST_CHECK_EQUAL(cause.id(), "1");
    BOOST_REQUIRE_EQUAL(disruption->tags_size(), 1);
    auto tag = disruption->tags(0);
    BOOST_CHECK_EQUAL(tag.id(), "1");
    BOOST_CHECK_EQUAL(tag.name(), "name");
    BOOST_CHECK_EQUAL(tag.created_at(), 11);
    BOOST_CHECK_EQUAL(tag.updated_at(), 22);
    BOOST_REQUIRE_EQUAL(disruption->impacts_size(), 1);
    auto impact = disruption->impacts(0);
    BOOST_CHECK_EQUAL(impact.id(), "1");
    BOOST_CHECK_EQUAL(impact.created_at(), 11);
    BOOST_CHECK_EQUAL(impact.updated_at(), 22);
    auto severity = impact.severity();
    BOOST_CHECK_EQUAL(severity.id(), "2");
    BOOST_REQUIRE_EQUAL(impact.application_periods_size(), 1);
    auto application = impact.application_periods(0);
    BOOST_CHECK_EQUAL(application.start(), 1);
    BOOST_REQUIRE_EQUAL(impact.informed_entities_size(), 2);
    auto ptobject = impact.informed_entities(0);
    BOOST_CHECK_EQUAL(ptobject.uri(), "uri");
    ptobject = impact.informed_entities(1);
    BOOST_CHECK_EQUAL(ptobject.uri(), "uri2");
}

BOOST_AUTO_TEST_CASE(two_properties) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);

    Const_it const_it;
    const_it.set_disruption("1", "22", "33", "44", "55",
            "note", "reference");
    const_it.set_cause("1", "wording", "11", "22");
    const_it.set_tag("1", "name", "11", "22");
    const_it.set_impact("1", "11", "22");
    const_it.set_severity("2", "wording", "22", "33",
            "blocking", "color", "2");
    const_it.set_application_period("0", "1", "2");
    const_it.set_ptobject("id", "uri", "line", "1", "2");
    const_it.set_property("foo", "bar", "31");
    reader(const_it);
    const_it.set_property("fo", "obar", "31");
    reader(const_it);
    const_it.set_property("foo", "bar", "31");
    reader(const_it);
    const auto& disruption = reader.disruption;
    BOOST_CHECK_EQUAL(disruption->id(), "1");
    auto cause = disruption->cause();
    BOOST_CHECK_EQUAL(cause.id(), "1");
    BOOST_REQUIRE_EQUAL(disruption->tags_size(), 1);
    auto tag = disruption->tags(0);
    BOOST_CHECK_EQUAL(tag.id(), "1");
    BOOST_CHECK_EQUAL(tag.name(), "name");
    BOOST_CHECK_EQUAL(tag.created_at(), 11);
    BOOST_CHECK_EQUAL(tag.updated_at(), 22);
    BOOST_REQUIRE_EQUAL(disruption->impacts_size(), 1);
    auto impact = disruption->impacts(0);
    BOOST_CHECK_EQUAL(impact.id(), "1");
    BOOST_CHECK_EQUAL(impact.created_at(), 11);
    BOOST_CHECK_EQUAL(impact.updated_at(), 22);
    auto severity = impact.severity();
    BOOST_CHECK_EQUAL(severity.id(), "2");
    BOOST_REQUIRE_EQUAL(impact.application_periods_size(), 1);
    auto application = impact.application_periods(0);
    BOOST_CHECK_EQUAL(application.start(), 1);
    BOOST_REQUIRE_EQUAL(impact.informed_entities_size(), 1);
    auto ptobject = impact.informed_entities(0);
    BOOST_CHECK_EQUAL(ptobject.uri(), "uri");
    BOOST_REQUIRE_EQUAL(reader.disruption->properties_size(), 2);
    auto property = reader.disruption->properties(0);
    BOOST_CHECK_EQUAL(property.key(), "foo");
    BOOST_CHECK_EQUAL(property.type(), "bar");
    BOOST_CHECK_EQUAL(property.value(), "31");
    property = reader.disruption->properties(1);
    BOOST_CHECK_EQUAL(property.key(), "fo");
    BOOST_CHECK_EQUAL(property.type(), "obar");
    BOOST_CHECK_EQUAL(property.value(), "31");
}

BOOST_AUTO_TEST_CASE(message) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);
    reader.disruption = std::make_unique<chaos::Disruption>();

    // Add an impact
    Const_it const_it;
    const_it.set_message("m_id_1", "message_text_web", "1", "2");
    reader.impact = new chaos::Impact();
    // Add a message with channel web
    auto* message = reader.impact->add_messages();
    reader.fill_message(const_it, message);
    BOOST_REQUIRE_EQUAL(reader.impact->messages_size(), 1);
    BOOST_CHECK_EQUAL(message->text(), "message_text_web");
    BOOST_CHECK_EQUAL(message->created_at(), 1);
    BOOST_CHECK_EQUAL(message->updated_at(), 2);

    //Add a channel with a channel_type web
    const_it.set_channel("channel_id", "channel_name_web", "type","400" ,"1", "2");
    auto* channel = message->mutable_channel();
    reader.fill_channel(const_it, channel);
    BOOST_REQUIRE(message->mutable_channel() != nullptr);
    BOOST_CHECK_EQUAL(channel->name(), "channel_name_web");
    BOOST_CHECK_EQUAL(channel->content_type(), "type");
    BOOST_CHECK_EQUAL(channel->max_size(), 400);
    BOOST_CHECK_EQUAL(channel->created_at(), 1);
    BOOST_CHECK_EQUAL(channel->updated_at(), 2);
    const_it.set_channel_type("id_1", "web");
    reader.fill_channel_type(const_it, channel);
    BOOST_REQUIRE_EQUAL(channel->types_size(), 1);

    // Add a message for mobile with channel and chanel_type
    const_it.set_message("m_id_2", "message_text_mobile", "1", "2");
    message = reader.impact->add_messages();
    reader.fill_message(const_it, message);
    BOOST_REQUIRE_EQUAL(reader.impact->messages_size(), 2);
    BOOST_CHECK_EQUAL(message->text(), "message_text_mobile");
    const_it.set_channel("channel_id_2", "channel_name_mobile", "type","400" ,"1", "2");
    channel = message->mutable_channel();
    reader.fill_channel(const_it, channel);
    const_it.set_channel_type("id_2", "mobile");
    reader.fill_channel_type(const_it, channel);
    BOOST_REQUIRE_EQUAL(channel->types_size(), 1);
    BOOST_REQUIRE_EQUAL(reader.impact->messages_size(), 2);

    // Add a message for notification with channel and chanel_type
    const_it.set_message("m_id_3", "message_text_notification", "1", "2");
    message = reader.impact->add_messages();
    reader.fill_message(const_it, message);
    BOOST_REQUIRE_EQUAL(reader.impact->messages_size(), 3);
    BOOST_CHECK_EQUAL(message->text(), "message_text_notification");
    const_it.set_channel("channel_id_3", "channel_name_notification", "type","400" ,"1", "2");
    channel = message->mutable_channel();
    reader.fill_channel(const_it, channel);
    const_it.set_channel_type("id_3", "notification");
    reader.fill_channel_type(const_it, channel);
    BOOST_REQUIRE_EQUAL(channel->types_size(), 1);
}

/* a badly sorted disruption record example
 * Disruption record sorted in this way does not work for channel_type but works well for application_periods
 * ..., message_1 , channel_1, channel_type_1, application_period_1 ...
 * ..., message_2 , channel_2, channel_type_2, application_period_2 ...
 * ..., message_3 , channel_3, channel_type_3, ...
 * ..., message_1 , channel_1, channel_type_1, application_period_1 ...
 * ..., message_2 , channel_2, channel_type_2, application_period_2 ...
 * ..., message_3 , channel_3, channel_type_3, ...
 */
BOOST_AUTO_TEST_CASE(disruption_badly_sorted_informations) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);

    Const_it const_it;
    const_it.set_disruption("1", "22", "33", "44", "55", "note", "reference");
    const_it.set_cause("1", "wording", "11", "22");
    const_it.set_tag("1", "name", "11", "22");
    const_it.set_impact("1", "11", "22");
    const_it.set_severity("2", "wording", "22", "33", "blocking", "color", "2");
    const_it.set_application_period("0", "1", "2");
    const_it.set_ptobject("id_1", "uri_1", "line", "1", "2");
    reader(const_it);

    // Combination of message, channel and channel_type
    const_it.set_message("m_id_1", "message_text_web", "1", "2");
    const_it.set_channel("channel_id", "channel_name_web", "type","400" ,"1", "2");
    const_it.set_channel_type("id_1", "web");
    reader(const_it);
    const_it.set_application_period("1", "2", "3");
    const_it.set_ptobject("id_2", "uri_2", "line", "1", "2");
    const_it.set_message("m_id_2", "message_text_mobile", "1", "2");
    const_it.set_channel("channel_id_2", "channel_name_mobile", "type","400" ,"1", "2");
    const_it.set_channel_type("id_2", "mobile");
    reader(const_it);

    const_it.set_message("m_id_3", "message_text_notification", "1", "2");
    const_it.set_channel("channel_id_3", "channel_name_notification", "type","400" ,"1", "2");
    const_it.set_channel_type("id_3", "notification");
    reader(const_it);

    // Duplicate combination of message, channel and channel_type
    const_it.set_application_period("0", "1", "2");
    const_it.set_ptobject("id_1", "uri_1", "line", "1", "2");
    const_it.set_message("m_id_1", "message_text_web", "1", "2");
    const_it.set_channel("channel_id", "channel_name_web", "type","400" ,"1", "2");
    const_it.set_channel_type("id_1", "web");
    reader(const_it);
    const_it.set_application_period("1", "2", "3");
    const_it.set_ptobject("id_2", "uri_2", "line", "1", "2");
    const_it.set_message("m_id_2", "message_text_mobile", "1", "2");
    const_it.set_channel("channel_id_2", "channel_name_mobile", "type","400" ,"1", "2");
    const_it.set_channel_type("id_2", "mobile");
    reader(const_it);
    const_it.set_message("m_id_3", "message_text_notification", "1", "2");
    const_it.set_channel("channel_id_3", "channel_name_notification", "type","400" ,"1", "2");
    const_it.set_channel_type("id_3", "notification");
    reader(const_it);

    const auto& disruption = reader.disruption;
    BOOST_CHECK_EQUAL(disruption->id(), "1");
    auto cause = disruption->cause();
    BOOST_CHECK_EQUAL(cause.id(), "1");
    BOOST_CHECK_EQUAL(disruption->tags_size(), 1);
    BOOST_REQUIRE_EQUAL(disruption->impacts_size(), 1);
    auto impact = disruption->impacts(0);
    BOOST_CHECK_EQUAL(impact.id(), "1");
    auto severity = impact.severity();
    BOOST_CHECK_EQUAL(severity.id(), "2");
    BOOST_CHECK_EQUAL(impact.application_periods_size(), 2);
    BOOST_CHECK_EQUAL(impact.informed_entities_size(), 2);
    BOOST_REQUIRE_EQUAL(impact.messages_size(), 3);
    // Message web
    auto message = impact.messages(0);
    BOOST_CHECK_EQUAL(message.text(), "message_text_web");
    auto channel = message.channel();
    BOOST_CHECK_EQUAL(channel.name(), "channel_name_web");
    BOOST_REQUIRE_EQUAL(channel.types_size(), 1);
    auto channel_type = channel.types(0);
    BOOST_CHECK_EQUAL(channel_type, chaos::Channel_Type_web);
    // Message mobile
    message = impact.messages(1);
    BOOST_CHECK_EQUAL(message.text(), "message_text_mobile");
    channel = message.channel();
    BOOST_CHECK_EQUAL(channel.name(), "channel_name_mobile");
    BOOST_REQUIRE_EQUAL(channel.types_size(), 1);
    channel_type = channel.types(0);
    BOOST_CHECK_EQUAL(channel_type, chaos::Channel_Type_mobile);

    // Message notification
    message = impact.messages(2);
    BOOST_CHECK_EQUAL(message.text(), "message_text_notification");
    channel = message.channel();
    BOOST_CHECK_EQUAL(channel.name(), "channel_name_notification");
    // Here we have 4 channel_types instead of 1. This is an error
    BOOST_REQUIRE_EQUAL(channel.types_size(), 4);
    channel_type = channel.types(0);
    BOOST_CHECK_EQUAL(channel_type, chaos::Channel_Type_notification);
}

/* a well sorted disruption record example
 * Disruption record sorted in this way works for channel_type as well as for application_periods
 * ..., message_1 , channel_1, channel_type_1, application_period_1 ...
 * ..., message_1 , channel_1, channel_type_1, application_period_1 ...
 * ..., message_2 , channel_2, channel_type_2, application_period_2 ...
 * ..., message_2 , channel_2, channel_type_2, application_period_2 ...
 * ..., message_3 , channel_3, channel_type_3, ...
 * ..., message_3 , channel_3, channel_type_3, ...
 */
BOOST_AUTO_TEST_CASE(disruption_well_sorted_informations) {
    navitia::type::PT_Data pt_data;
    navitia::type::MetaData meta;
    navitia::DisruptionDatabaseReader reader(pt_data, meta);

    Const_it const_it;
    const_it.set_disruption("1", "22", "33", "44", "55", "note", "reference");
    const_it.set_cause("1", "wording", "11", "22");
    const_it.set_tag("1", "name", "11", "22");
    const_it.set_impact("1", "11", "22");
    const_it.set_severity("2", "wording", "22", "33", "blocking", "color", "2");
    const_it.set_application_period("0", "1", "2");
    const_it.set_ptobject("id_1", "uri_1", "line", "1", "2");
    reader(const_it);

    // First combination of message, channel and channel_type with duplicate values
    const_it.set_application_period("0", "1", "2");
    const_it.set_ptobject("id_1", "uri_1", "line", "1", "2");
    const_it.set_message("m_id_1", "message_text_web", "1", "2");
    const_it.set_channel("channel_id", "channel_name_web", "type","400" ,"1", "2");
    const_it.set_channel_type("id_1", "web");
    reader(const_it);
    const_it.set_message("m_id_1", "message_text_web", "1", "2");
    const_it.set_channel("channel_id", "channel_name_web", "type","400" ,"1", "2");
    const_it.set_channel_type("id_1", "web");
    reader(const_it);

    // Second combination of message, channel and channel_type with duplicate values
    const_it.set_application_period("1", "2", "3");
    const_it.set_ptobject("id_2", "uri_2", "line", "1", "2");
    const_it.set_message("m_id_2", "message_text_mobile", "1", "2");
    const_it.set_channel("channel_id_2", "channel_name_mobile", "type","400" ,"1", "2");
    const_it.set_channel_type("id_2", "mobile");
    reader(const_it);
    const_it.set_application_period("1", "2", "3");
    const_it.set_ptobject("id_2", "uri_2", "line", "1", "2");
    const_it.set_message("m_id_2", "message_text_mobile", "1", "2");
    const_it.set_channel("channel_id_2", "channel_name_mobile", "type","400" ,"1", "2");
    const_it.set_channel_type("id_2", "mobile");
    reader(const_it);

    // Third combination of message, channel and channel_type with duplicate values
    const_it.set_message("m_id_3", "message_text_notification", "1", "2");
    const_it.set_channel("channel_id_3", "channel_name_notification", "type","400" ,"1", "2");
    const_it.set_channel_type("id_3", "notification");
    reader(const_it);
    const_it.set_message("m_id_3", "message_text_notification", "1", "2");
    const_it.set_channel("channel_id_3", "channel_name_notification", "type","400" ,"1", "2");
    const_it.set_channel_type("id_3", "notification");
    reader(const_it);

    const auto& disruption = reader.disruption;
    BOOST_CHECK_EQUAL(disruption->id(), "1");
    auto cause = disruption->cause();
    BOOST_CHECK_EQUAL(cause.id(), "1");
    BOOST_CHECK_EQUAL(disruption->tags_size(), 1);
    BOOST_REQUIRE_EQUAL(disruption->impacts_size(), 1);
    auto impact = disruption->impacts(0);
    BOOST_CHECK_EQUAL(impact.id(), "1");
    auto severity = impact.severity();
    BOOST_CHECK_EQUAL(severity.id(), "2");
    BOOST_CHECK_EQUAL(impact.application_periods_size(), 2);
    BOOST_CHECK_EQUAL(impact.informed_entities_size(), 2);
    BOOST_REQUIRE_EQUAL(impact.messages_size(), 3);
    // Message web
    auto message = impact.messages(0);
    BOOST_CHECK_EQUAL(message.text(), "message_text_web");
    auto channel = message.channel();
    BOOST_CHECK_EQUAL(channel.name(), "channel_name_web");
    BOOST_REQUIRE_EQUAL(channel.types_size(), 1);
    auto channel_type = channel.types(0);
    BOOST_CHECK_EQUAL(channel_type, chaos::Channel_Type_web);
    // Message mobile
    message = impact.messages(1);
    BOOST_CHECK_EQUAL(message.text(), "message_text_mobile");
    channel = message.channel();
    BOOST_CHECK_EQUAL(channel.name(), "channel_name_mobile");
    BOOST_REQUIRE_EQUAL(channel.types_size(), 1);
    channel_type = channel.types(0);
    BOOST_CHECK_EQUAL(channel_type, chaos::Channel_Type_mobile);

    // Message notification
    message = impact.messages(2);
    BOOST_CHECK_EQUAL(message.text(), "message_text_notification");
    channel = message.channel();
    BOOST_CHECK_EQUAL(channel.name(), "channel_name_notification");
    BOOST_REQUIRE_EQUAL(channel.types_size(), 1);
    channel_type = channel.types(0);
    BOOST_CHECK_EQUAL(channel_type, chaos::Channel_Type_notification);
}
