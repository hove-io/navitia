import pybreaker
import logging
from jormungandr import utils
from jormungandr.transient_socket import TransientSocket


class ZmqBackend:
    def __init__(
        self,
        socket: TransientSocket,
        timeout: int,
        breaker: pybreaker.CircuitBreaker,
        connector_type_for_error_reporting: str,
        connector_name_for_error_reporting: str,
    ):
        self.timeout = timeout
        self.socket = socket
        self.breaker = breaker
        self.timeout = timeout
        self.connector_type_for_error_reporting = connector_type_for_error_reporting
        self.connector_name_for_error_reporting = connector_name_for_error_reporting
        self.logger = logging.getLogger()

    def call(self, request: str) -> str:
        try:
            return self.breaker.call(lambda: self.socket.call(request, self.timeout))

        except pybreaker.CircuitBreakerError as e:
            self.logger.error(
                f'{self.connector_type_for_error_reporting}:{self.connector_name_for_error_reporting} service dead (error: {e})'
            )
            utils.record_external_failure(
                'circuit breaker open',
                self.connector_type_for_error_reporting,
                self.connector_name_for_error_reporting,
            )
            raise
        except Exception as e:
            self.logger.exception(
                f'{self.connector_type_for_error_reporting}:{self.connector_name_for_error_reporting} error'
            )
            utils.record_external_failure(
                str(e), self.connector_type_for_error_reporting, self.connector_name_for_error_reporting
            )
            raise
