

class Instance(object):
    def __init__(self, config):
        self.name = config['instance']['name']

        self.source_directory = config['instance']['source-directory']
        self.backup_directory = config['instance']['backup-directory']
        self.tmp_file = config['instance']['tmp-file']
        self.target_file = config['instance']['target-file']
        self.rt_topics = config['instance']['rt-topics']
        self.exchange = config['instance']['exchange']
        self.synonyms_file = config['instance']['synonyms_file']
        self.aliases_file = config['instance']['aliases_file']

        self.pg_host = config['database']['host']
        self.pg_dbname = config['database']['dbname']
        self.pg_username = config['database']['username']
        self.pg_password = config['database']['password']

    def __repr__(self):
        return '<Instance %s>' % self.name
