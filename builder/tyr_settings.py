CELERY_BROKER_URL = 'amqp://guest:guest@rabbitmq:5672//'

SQLALCHEMY_DATABASE_URI = 'postgresql://navitia:navitia@database/jormungandr'

# If the presence of the cities db is not needed, un-comment the following line
# CITIES_DATABASE_URI = None

REDIS_HOST = 'redis'

INSTANCES_DIR = '/etc/tyr.d'
