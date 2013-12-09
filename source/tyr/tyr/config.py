import logging


def configure_logger(app):
    """
    configuration du logging
    """
    filename = app.config["LOG_FILENAME"]
    handler = logging.FileHandler(filename)
    log_format = '[%(asctime)s] [%(levelname)s] [%(name)s] - %(message)s - ' \
            '%(filename)s:%(lineno)d in %(funcName)s'
    handler.setFormatter(logging.Formatter(log_format))
    app.logger.addHandler(handler)
    app.logger.setLevel(app.config["LOG_LEVEL"])

    logging.getLogger('sqlalchemy.engine').setLevel(
            app.config["LOG_LEVEL_SQLALCHEMY"])
    logging.getLogger('sqlalchemy.pool').setLevel(
            app.config["LOG_LEVEL_SQLALCHEMY"])
    logging.getLogger('sqlalchemy.dialects.postgresql')\
            .setLevel(app.config["LOG_LEVEL_SQLALCHEMY"])

    logging.getLogger('sqlalchemy.engine').addHandler(handler)
    logging.getLogger('sqlalchemy.pool').addHandler(handler)
    logging.getLogger('sqlalchemy.dialects.postgresql').addHandler(handler)
