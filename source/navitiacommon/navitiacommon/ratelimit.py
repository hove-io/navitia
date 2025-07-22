# Copyright (c) 2013 Jason Foote, DomainTools LLC
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# this file is from: https://github.com/DomainTools/rate-limit
"""
This module implements a RateLimiter class.
RateLimiter is a Redis backed object used to define one or more rules to rate limit requests.

This module can be run to show an example of a running RateLimiter instance.
"""

import logging
import math
import redis
import time
from builtins import zip


class RateLimiter(object):
    """
    RateLimiter is used to define one or more rate limit rules.
    These rules are checked on .acquire() and we either return True or False based on if we can make the request,
    or we can block until we make the request.
    Manual blocks are also supported with the block method.
    """

    def __init__(
        self,
        conditions=None,
        redis_host='localhost',
        redis_port=6379,
        redis_db=0,
        redis_password=None,
        redis_namespace='ratelimiter',
    ):
        """
        Initalize an instance of a RateLimiter

        conditions - list or tuple of rate limit rules
        redis_host - Redis host to use
        redis_port - Redis port (if different than default 6379)
        redis_db   - Redis DB to use (if different than 0)
        redis_password - Redis password (if needed)
        redis_namespace - Redis key namespace
        """

        self.redis = redis.Redis(host=redis_host, port=redis_port, db=redis_db, password=redis_password)
        self.log = logging.getLogger('RateLimiter')
        self.namespace = redis_namespace
        self.conditions = []
        self.list_ttl = 0

        if conditions:
            self.add_condition(*conditions)

    def add_condition(self, *conditions):
        """
        Adds one or more conditions to this RateLimiter instance
        Conditions can be given as:
            add_condition(1, 10)
            add_condition((1, 10))
            add_condition((1, 10), (30, 600))
            add_condition({'requests': 1, 'seconds': 10})
            add_condition({'requests': 1, 'seconds': 10}, {'requests': 200, 'hours': 6})

            dict can contain 'seconds', 'minutes', 'hours', and 'days' time period parameters
        """
        # allow add_condition(1,2) as well as add_condition((1,2))
        if len(conditions) == 2 and isinstance(conditions[0], int):
            conditions = [conditions]

        for condition in conditions:
            if isinstance(condition, dict):
                requests = condition['requests']
                seconds = condition.get('seconds', 0) + (
                    60
                    * (
                        condition.get('minutes', 0)
                        + 60 * (condition.get('hours', 0) + 24 * condition.get('days', 0))
                    )
                )
            else:
                requests, seconds = condition

            # requests and seconds always a positive integer
            requests = int(requests)
            seconds = int(seconds)

            if requests < 0:
                raise ValueError('negative number of requests (%s)' % requests)
            if seconds < 0:
                raise ValueError('negative time period given (%s)' % seconds)

            if seconds > 0:
                if requests == 0:
                    self.log.warning('added block all condition (%s/%s)', requests, seconds)
                else:
                    self.log.debug('added condition (%s/%s)', requests, seconds)

                self.conditions.append((requests, seconds))

                if seconds > self.list_ttl:
                    self.list_ttl = seconds
            else:
                self.log.warning('time period of 0 seconds. not adding condition')

        # sort by requests so we query redis list in order as well as know max and min requests by position
        self.conditions.sort()

    def block(self, key, seconds=0, minutes=0, hours=0, days=0):
        """
        Set manual block for key for a period of time
        key - key to track what to rate limit
        Time parameters are added together and is the period to block for
            seconds
            minutes
            hours
            days
        """
        seconds = seconds + 60 * (minutes + 60 * (hours + 24 * days))
        # default to largest time period we are limiting by
        if not seconds:
            seconds = self.list_ttl

            if not seconds:
                self.log.warning('block called but no default block time. not blocking')
                return 0

        if not isinstance(seconds, int):
            seconds = int(math.ceil(seconds))

        key = ':'.join((self.namespace, key, 'block'))
        self.log.warning('block key (%s) for %ds', key, seconds)
        with self.redis.pipeline() as pipe:
            pipe.set(key, '1')
            pipe.expire(key, seconds)
            pipe.execute()

        return seconds

    def acquire(self, key, block=True):
        """
        Tests whether we can make a request, or if we are currently being limited
        key - key to track what to rate limit
        block - Whether to wait until we can make the request
        """
        if block:
            while True:
                success, wait = self._make_ping(key)
                if success:
                    return True
                self.log.debug('blocking acquire sleeping for %.1fs', wait)
                time.sleep(wait)
        else:
            success, wait = self._make_ping(key)
            return success

    # alternative acquire interface ratelimiter(key)
    __call__ = acquire

    def _make_ping(self, key):

        # shortcut if no configured conditions
        if not self.conditions:
            return True, 0.0

        # short cut if we are limiting to 0 requests
        min_requests, min_request_seconds = self.conditions[0]
        if min_requests == 0:
            self.log.warning('(%s) hit block all limit (%s/%s)', key, min_requests, min_request_seconds)
            return False, min_request_seconds

        log_key = ':'.join((self.namespace, key, 'log'))
        block_key = ':'.join((self.namespace, key, 'block'))
        lock_key = ':'.join((self.namespace, key, 'lock'))

        with self.redis.lock(lock_key, timeout=10):

            with self.redis.pipeline() as pipe:
                for requests, _ in self.conditions:
                    pipe.lindex(log_key, requests - 1)  # subtract 1 as 0 indexed

                # check manual block keys
                pipe.ttl(block_key)
                pipe.get(block_key)
                boundry_timestamps = pipe.execute()

            blocked = boundry_timestamps.pop()
            block_ttl = boundry_timestamps.pop()

            if blocked is not None:
                # block_ttl is None for last second of a keys life. set min of 0.5
                if block_ttl is None:
                    block_ttl = 0.5
                self.log.warning('(%s) hit manual block. %ss remaining', key, block_ttl)
                return False, block_ttl

            timestamp = time.time()

            for boundry_timestamp, (requests, seconds) in zip(boundry_timestamps, self.conditions):
                # if we dont yet have n number of requests boundry_timestamp will be None and this condition wont be limiting
                if boundry_timestamp is not None:
                    boundry_timestamp = float(boundry_timestamp)
                    if boundry_timestamp + seconds > timestamp:
                        self.log.warning(
                            '(%s) hit limit (%s/%s) time to allow %.1fs',
                            key,
                            requests,
                            seconds,
                            boundry_timestamp + seconds - timestamp,
                        )
                        return False, boundry_timestamp + seconds - timestamp

            # record our success
            with self.redis.pipeline() as pipe:
                pipe.lpush(log_key, timestamp)
                max_requests, _ = self.conditions[-1]
                pipe.ltrim(log_key, 0, max_requests - 1)  # 0 indexed so subtract 1
                # if we never use this key again, let it fall out of the DB after max seconds has past
                pipe.expire(log_key, self.list_ttl)
                pipe.execute()

        return True, 0.0


if __name__ == '__main__':
    """
    This is an example of rate limiting using the RateLimiter class
    """
    import sys

    logging.basicConfig(
        format='%(asctime)s %(process)s %(levelname)s %(name)s %(message)s',
        level=logging.DEBUG,
        stream=sys.stdout,
    )
    log = logging.getLogger('ratelimit.main')
    key = 'TestRateLimiter'

    rate = RateLimiter(conditions=((1, 1), (2, 5)))
    rate.add_condition((3, 10), (4, 15))
    rate.add_condition({'requests': 20, 'minutes': 5})
    rate.add_condition({'requests': 40, 'minutes': 15}, {'requests': 400, 'days': 1})

    i = 1
    for _ in range(0, 5):
        rate.acquire(key)
        log.info('***************     ping %d     ***************', i)
        i += 1

    rate.block(key, seconds=10)
    for _ in range(0, 10):
        rate.acquire(key)
        log.info('***************     ping %d     ***************', i)
        i += 1

    # block all keys
    rate.add_condition(0, 1)

    for _ in range(0, 5):
        rate(key, block=False)  # alternative interface
        time.sleep(1)


class FakeRateLimiter(object):
    """
    emulate a ratelimiter but do nothing :)
    """

    def __init__(self, *args, **kwargs):
        pass

    def acquire(self, *args, **kwargs):
        return True
