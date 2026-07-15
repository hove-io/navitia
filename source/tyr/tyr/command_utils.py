# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

# Flask-Script is no longer maintained and is incompatible with Flask 3.
# This module provides a thin compatibility layer, backed by the click-based
# Flask CLI, so that the existing tyr commands can be registered without
# rewriting each of them. Commands are exposed through the application CLI
# (``manage_tyr.py <command>``) and run within an application context.

from __future__ import absolute_import
import functools
import inspect
import click
from flask.cli import with_appcontext


class Option(object):
    """Flask-Script compatible option, translated into a click parameter.

    A positional argument is declared with only a ``dest`` keyword, an option
    with one or several option strings ('-n', '--name'). ``action='store_true'``
    (resp. 'store_false') maps to a boolean flag.
    """

    def __init__(self, *args, **kwargs):
        self.option_strings = [a for a in args if isinstance(a, str) and a.startswith('-')]
        positional = [a for a in args if isinstance(a, str) and not a.startswith('-')]
        self.dest = kwargs.get('dest') or (positional[0] if positional else None)
        self.default = kwargs.get('default')
        self.help = kwargs.get('help')
        self.action = kwargs.get('action')

    def to_click(self):
        if not self.option_strings:
            return click.Argument([self.dest])
        decls = list(self.option_strings)
        if self.dest:
            decls.append(self.dest)
        if self.action == 'store_true':
            return click.Option(decls, is_flag=True, default=bool(self.default), help=self.help)
        if self.action == 'store_false':
            default = True if self.default is None else self.default
            return click.Option(decls, is_flag=True, default=default, help=self.help)
        return click.Option(decls, default=self.default, help=self.help)


class Command(object):
    """Flask-Script compatible base command.

    Subclasses define ``get_options`` (a list of :class:`Option`) and ``run``.
    """

    def get_options(self):
        return []

    def run(self, *args, **kwargs):
        raise NotImplementedError


def _params_from_signature(func):
    params = []
    for name, param in inspect.signature(func).parameters.items():
        if param.default is inspect.Parameter.empty:
            params.append(click.Argument([name]))
        elif isinstance(param.default, bool):
            params.append(click.Option(['--' + name], is_flag=True, default=param.default))
        else:
            params.append(click.Option(['--' + name], default=param.default))
    return params


class Manager(object):
    """Minimal Flask-Script ``Manager`` replacement backed by the Flask CLI."""

    def __init__(self, app):
        self.app = app

    def command(self, func):
        """Register ``func`` as a CLI command, deriving parameters from its signature.

        The original function is returned unchanged so it stays directly callable.
        """
        params = _params_from_signature(func)

        @with_appcontext
        @functools.wraps(func)
        def callback(*args, **kwargs):
            return func(*args, **kwargs)

        self.app.cli.add_command(
            click.Command(func.__name__, params=params, callback=callback, help=func.__doc__)
        )
        return func

    def add_command(self, name, command):
        """Register a :class:`Command` instance under ``name``."""
        params = [option.to_click() for option in command.get_options()]

        @with_appcontext
        def callback(**kwargs):
            return command.run(**kwargs)

        self.app.cli.add_command(click.Command(name, params=params, callback=callback, help=command.__doc__))
