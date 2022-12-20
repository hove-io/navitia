# coding: utf-8
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

"""
Function to launch a bin
"""
from __future__ import absolute_import, unicode_literals

import subprocess
import os
import select
import re
import fcntl
import errno


class LogLine(object):
    def __init__(self, line):
        if line.startswith('DEBUG') or line.startswith('TRACE') or line.startswith('NOTICE') or not line:
            self.level = 10
        elif line.startswith('INFO'):
            self.level = 20
        elif line.startswith('WARN'):
            self.level = 30
        else:
            self.level = 40

        pos = line.find(' - ')
        if 0 < pos < 10:
            self.msg = line[pos + 3 :]
        else:
            self.msg = line


def parse_log(buff):
    logs = []
    line, sep, buff = buff.partition('\n')
    while sep and line:
        logs.append(LogLine(line))
        line, sep, buff = buff.partition('\n')
    if not sep:
        buff = line  # we put back the last unterminated line in the buffer
    return logs, buff


# from: http://stackoverflow.com/questions/7729336/how-can-i-print-and-display-subprocess-stdout-and-stderr-output-without-distorti/7730201#7730201
def make_async(fd):
    fcntl.fcntl(fd, fcntl.F_SETFL, fcntl.fcntl(fd, fcntl.F_GETFL) | os.O_NONBLOCK)


# Helper function to read some data from a file descriptor, ignoring EAGAIN errors
# (those errors mean that there are no data available for the moment)
def read_async(fd):
    try:
        return fd.read()
    except IOError as e:
        if e.errno != errno.EAGAIN:
            raise e
        else:
            return ''


def hide_args(whole_args, args_to_be_hidden):
    """hide the given args' contents.

    >>> hide_args(["--toto", "12345", "--titi", "my almighty password", "--tati", "hid args"], ["--titi", "--tati"])
    ['--toto', '12345', '--titi', 'xxxxxxxxx', '--tati', 'xxxxxxxxx']
    """
    args_copy = list(whole_args)
    for to_be_hidden in args_to_be_hidden:
        if args_copy.count(to_be_hidden) != 0:
            i = args_copy.index(to_be_hidden)
            args_copy[i + 1] = "xxxxxxxxx"
    return args_copy


def launch_exec_traces(exec_name, args, logger):
    """Launch an exec with args, log the outputs"""
    hidden_args = hide_args(args, ["--cities-connection-string", "--connection-string"])
    log = 'Launching ' + exec_name + ' ' + ' '.join(hidden_args)
    # we hide the password in logs
    logger.info(re.sub('password=\w+', 'password=xxxxxxxxx', log))

    args.insert(0, exec_name)

    proc = subprocess.Popen(args, stderr=subprocess.PIPE, stdout=subprocess.PIPE, close_fds=True)
    traces = ""
    try:
        make_async(proc.stderr)
        make_async(proc.stdout)
        while True:
            select.select([proc.stdout, proc.stderr], [], [])

            try:
                for pipe in proc.stdout, proc.stderr:
                    log_pipe = read_async(pipe)
                    if log_pipe:
                        logs, line = parse_log(log_pipe.decode("utf-8", errors="replace"))
                        for l in logs:
                            logger.log(l.level, l.msg)
                            traces += "##  {}  ##".format(l.msg)
            except Exception as e:
                logger.error("error in forwarding logs but process continuing: {}".format(e))

            if proc.poll() is not None:
                break
    finally:
        proc.stdout.close()
        proc.stderr.close()

    return proc.returncode, traces


def launch_exec(exec_name, args, logger):
    code, _ = launch_exec_traces(exec_name, args, logger)
    return code
