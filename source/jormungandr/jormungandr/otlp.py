# coding=utf-8

# Copyright (c) 2001, Canal TP and/or its affiliates. All rights reserved.
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
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# [matrix] channel #navitia:matrix.org (https://app.element.io/#/room/#navitia:matrix.org)
# https://groups.google.com/d/forum/navitia
# www.navitia.io

import os
import logging
from typing import Dict
from flask import request

from opentelemetry.sdk.resources import SERVICE_NAME, Resource
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.exporter.otlp.proto.http.metric_exporter import OTLPMetricExporter
from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
from opentelemetry.exporter.otlp.proto.http.trace_exporter import OTLPSpanExporter
from opentelemetry.trace.status import Status, StatusCode
from opentelemetry import trace, metrics


class Otlp:
    __service_name: str = "jormungandr"
    __platform: str = "unknown"
    __account: str = "unknown"
    __labels: Dict = {}

    def __init__(self, platform: str, account: str) -> None:
        self.__log = logging.getLogger(__name__)
        self._tracer = None
        self._meter = None
        otel_exporter_otlp_endpoint = os.getenv("OTEL_EXPORTER_OTLP_ENDPOINT", "")

        if otel_exporter_otlp_endpoint == "":
            self.__log.info("OTLP not configured. Disabling otlp.")
            return

        try:
            self.__platform = platform + " (Python)"
            self.__account = account
            self.__resource = Resource(
                attributes={
                    SERVICE_NAME: self.__service_name,
                }
            )
            self.__init_tracer()
            self.__init_meter()

            self.__declare_counters()
            self.__declare_histograms()
        except Exception:
            self.__log.exception("Failure while initializing otlp. Disabling otlp.")
            self._tracer = None
            self._meter = None

    def __init_tracer(self):
        trace_exporter = OTLPSpanExporter()
        span_processor = BatchSpanProcessor(trace_exporter)
        trace_provider = TracerProvider(resource=self.__resource)

        trace_provider.add_span_processor(span_processor)
        trace.set_tracer_provider(trace_provider)

        self._tracer = trace.get_tracer(__name__)

    def __init_meter(self):
        exporter = OTLPMetricExporter()
        reader = PeriodicExportingMetricReader(exporter)
        provider = MeterProvider(resource=self.__resource, metric_readers=[reader])

        metrics.set_meter_provider(provider)

        self._meter = metrics.get_meter(__name__)

    def __declare_counters(self) -> None:
        self.__jormungandr_exception = self._meter.create_counter(
            name="jormungandr_exception", description="Count exception"
        )
        self.__jormungandr_request_call = self._meter.create_counter(
            name="jormungandr_request_call", description="Count request call"
        )
        self.__jormungandr_event = self._meter.create_counter(
            name="jormungandr_event", description="Count event"
        )

    def __declare_histograms(self) -> None:
        self._meter = metrics.get_meter(__name__)

        self.__jormungandr_event_duration = self._meter.create_histogram(
            name="jormungandr_event_duration", description="Event duration"
        )
        self.__jormungandr_request_call_duration = self._meter.create_histogram(
            name="jormungandr_request_call_duration", description="Request call duration", unit="s"
        )

    def get_tracer(self) -> trace.Tracer:
        return self._tracer

    def __get_request_id(self) -> str:
        if not request:
            return "unknown"

        return str(request.id)

    def __should_ignore(self) -> bool:
        ignore_paths = ["/status", "/"]

        return request.path in ignore_paths

    def __get_labels(self) -> Dict:
        request_id = self.__get_request_id()

        if request_id not in self.__labels:
            self.__labels[request_id] = self.__generate_default_labels()

        return self.__labels[request_id]

    def record_labels(self, labels: Dict) -> None:
        if self.__should_ignore():
            return

        self.__get_labels().update(labels)

    def record_label(self, label_name: str, label_value: str) -> None:
        if self.__should_ignore():
            return

        self.__get_labels()[label_name] = label_value

    def __clear_labels(self) -> None:
        self.__get_labels().clear()
        self.__labels.pop(self.__get_request_id(), None)

    def __generate_default_labels(self) -> Dict:
        return {
            "coverage": "unknown",
            "api": "unknown",
            "platform": self.__platform,
            "account": self.__account,
        }

    def __record_exception_trace(self, exception: BaseException, attributes: Dict = {}) -> None:
        try:
            with self._tracer.start_as_current_span("exception") as span:
                span.set_attribute("navitia_request_id", self.__get_request_id())
                for key, value in attributes.items():
                    span.set_attribute(key, value)
                for key, value in self.__get_labels().items():
                    span.set_attribute(key, value)
                span.set_status(Status(StatusCode.ERROR, "Exception"))
                span.record_exception(exception)
        except Exception:
            self.__log.exception("failure while reporting to otlp (with trace)")

    def __record_exception_meter(self, exception: BaseException) -> None:
        try:
            labels = {"exception_type": type(exception).__name__}
            for key, value in self.__get_labels().items():
                labels[key] = value

            self.__jormungandr_exception.add(1, labels)
        except Exception:
            self.__log.exception("failure while reporting to otlp (with meter)")

    def record_exception(self, exception: BaseException, attributes: Dict = {}) -> None:
        if self._tracer:
            self.__record_exception_trace(exception, attributes)

        if self._meter:
            self.__record_exception_meter(exception)

    def send_request_call_metrics(self, duration, labels=None) -> None:
        if not self._meter:
            return

        if labels:
            self.record_labels(labels)
        labels = self.__get_labels().copy()
        self.__jormungandr_request_call.add(1, labels)
        self.__jormungandr_request_call_duration.record(duration, labels)
        self.__clear_labels()

    def send_event_metrics(self, event_type: str, params: Dict = {}) -> None:
        if not self._meter:
            return

        labels = {"platform": self.__platform, "account": self.__account, "event_type": event_type}
        labels.update(params)

        if "navitia_request_id" in labels:
            labels.pop("navitia_request_id")
        if "duration" in labels:
            duration = labels.pop("duration", None)
            self.__jormungandr_event_duration.record(duration, labels)
        self.__jormungandr_event.add(1, labels)


otlp_instance = Otlp(os.getenv("OTEL_PLATFORM"), os.getenv("OTEL_ACCOUNT"))
