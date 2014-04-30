# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

def depth_argument(value, name):
    if value < 3:
        raise ValueError("The {} argument has to be <= 3, you gave : {}"
                         .format(name, value))
    return int(value)


def true_false(value, name):
    if value == "true":
        return True
    elif value == "false":
        return False
    else:
        raise ValueError("The {} argument must be true or false, you gave : {}"
                         .format(name, value))


def option_value(values):
    def to_return(value, name):
        if not (value in values):
            error = "The {} argument must be in list {}, you gave {}".\
                format(name, str(values), value)
            raise ValueError(error)
        return value
    return to_return
