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
import os


class OtlpMeta(type):
    _instance: Dict = {}

    def __call__(cls, *args, **kwds):
        if cls not in cls._instance:
            cls._instance[cls] = super().__call__(*args, **kwds)

        return cls._instance[cls]


class Otlp(metaclass=OtlpMeta):
    __service_name = "jormungandr"
    __request_call_labels = {}

    def __init__(self) -> None:
        self.__log = logging.getLogger(__name__)

        try:
            self.__environment = os.getenv("ENVIRONMENT", "local")
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
            self.__log.exception("failure while initializing otlp")
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
        pass

    def get_tracer(self) -> trace.Tracer:
        return self._tracer

    def record_exception(self, exception: BaseException, attributes: Dict = {}) -> None:
        """
        record the exception currently handled to otlp
        """
        if self._tracer:
            # TODO: Can remove this and use directly request.id below ?
            try:
                navitia_request_id = request.id
            except RuntimeError:
                self.__log.exception("failure while getting request id. We are outside of a flask context :(")
                navitia_request_id = 42

            try:
                span = trace.get_current_span()
                span.set_attribute("navitia_request_id", str(navitia_request_id))
                for key, value in attributes.items():
                    span.set_attribute(key, value)
                span.set_status(Status(StatusCode.ERROR, "Exception"))
                span.record_exception(exception)
            except Exception:
                self.__log.exception("failure while reporting to otlp (with trace)")

        if self._meter:
            try:
                self.__jormungandr_exception.add(
                    1,
                    {
                        "exception_type": type(exception).__name__,
                        "status": Status(StatusCode.ERROR, "Exception"),
                    },
                )
            except Exception:
                self.__log.exception("failure while reporting to otlp (with meter)")

    def send_request_call_metrics(self, labels=None) -> None:
        if labels:
            self.record_request_call_labels(labels)

        self.__request_call_labels["environment"] = self.__environment

        self.__jormungandr_request_call.add(1, self.__request_call_labels)
        self.__request_call_labels.clear()

    def record_request_call_labels(self, labels: Dict) -> None:
        self.__request_call_labels.update(labels)

    def record_request_call_label(self, label_name: str, label_value: str) -> None:
        self.__request_call_labels[label_name] = label_value

    def send_event_metric(self, event_type: str, labels: Dict = {}) -> None:
        labels["environment"] = self.__environment
        labels["event_type"] = event_type
        self.__jormungandr_event.add(1, labels)

    # def send_distributed_event(self, call_name: str, group_name: str, status) -> None:
    #     labels = {
    #         "service": service_name,
    #         "call": call_name,
    #         "group": group_name,
    #         "status": status,
    #         "az": os.getenv('JORMUNGANDR_DEPLOYMENT_AZ', "unknown"),
    #     }


otlp_instance = Otlp()


def __get_common_event_params(service_name, call_name, status="ok"):
    return {
        "service": service_name,
        "call": call_name,
        "status": status,
        "az": os.getenv('JORMUNGANDR_DEPLOYMENT_AZ', "unknown"),
    }


# # Using existing library of opentelemetry without instrumenting
# from opentelemetry.sdk.resources import SERVICE_NAME, Resource

# # Metrics part
# from opentelemetry import metrics
# from opentelemetry.sdk.metrics import MeterProvider
# from opentelemetry.exporter.otlp.proto.http.metric_exporter import OTLPMetricExporter
# from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader

# # Traces Part
# from opentelemetry import trace
# from opentelemetry.sdk.trace import TracerProvider
# from opentelemetry.sdk.trace.export import BatchSpanProcessor
# from opentelemetry.exporter.otlp.proto.http.trace_exporter import OTLPSpanExporter

# # OpenTelemetry configuration

# resource = Resource(
#     attributes={
#         SERVICE_NAME: "my_jormungandr_service",
#     }
# )

# # Metrics configuration
# exporter = OTLPMetricExporter()
# reader = PeriodicExportingMetricReader(exporter)

# provider = MeterProvider(resource=resource, metric_readers=[reader])
# metrics.set_meter_provider(provider)

# meter = metrics.get_meter("my_jormungandr_meter")
# my_metric_counter = meter.create_counter(name="my_jormungandr_from_otlp_lib", description="An example counter")
# my_metric_histogram = meter.create_histogram(
#     name="my_jormungandr_histogram_from_otlp_lib", description="An example histogram"
# )

# my_metric_counter.add(1, {"environment": "local"})


# # Trace configuration
# trace_exporter = OTLPSpanExporter()

# span_processor = BatchSpanProcessor(trace_exporter)

# trace_provider = TracerProvider(resource=resource)
# trace_provider.add_span_processor(span_processor)
# trace.set_tracer_provider(trace_provider)

# tracer = trace.get_tracer(__name__)


# my_trace = trace.get_tracer("my_jormungandr_tracer")


# @app.route("/my_trace")
# def call_http():
#     logging.info("-----------------------------------------------------")
#     my_metric_counter.add(1, {"environment": "local"})
#     my_metric_histogram.record(0.42, {"environment": "local"})

#     with tracer.start_as_current_span("example-span") as span:
#         span.set_attribute("example-attribute", "example-value")
#         print("Doing some work...")
#     logging.info("-----------------------------------------------------")

#     return 'good ?'
