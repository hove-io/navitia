# encoding: utf-8
#!/usr/bin/env python
import logging
import datetime
import time
from sqlalchemy import Column, Table, MetaData, select, create_engine, \
    ForeignKey, bindparam, and_, or_, exc
import connectors.task_pb2
import connectors.realtime_pb2
import connectors.type_pb2
import google
import os


def int_to_bitset(s):
    return str(s) if s <= 1 else int_to_bitset(s >> 1) + str(s & 1)


def get_pos_time(sql_time):
    return int(time.mktime(sql_time.timetuple()))


def get_datetime_to_second(sql_time):
    tt = sql_time.timetuple()
    return (tt.tm_hour * 60 * 60) + (tt.tm_min * 60) + tt.tm_sec


def get_navitia_type(object_type):
    if object_type == 'StopPoint':
        return connectors.type_pb2.STOP_POINT
    elif object_type == 'VehicleJourney':
        return connectors.type_pb2.VEHICLE_JOURNEY
    elif object_type == 'Line':
        return connectors.type_pb2.LINE
    elif object_type == 'Network':
        return connectors.type_pb2.NETWORK
    elif object_type == 'Route':
        return connectors.type_pb2.JOURNEY_PATTERN
    elif object_type == 'StopArea':
        return connectors.type_pb2.STOP_AREA
    elif object_type == 'RoutePoint':
        return connectors.type_pb2.JOURNEY_PATTERN_POINT
    else:
        return -1


class AtRealtimeReader(object):

    """
    Classe responsable de la lecture en base de donnee des evenements
    temps reel.
    """

    def __init__(self, config, redis_helper):
        self.message_list = []
        self.perturbation_list = []
        self.__engine = create_engine(
            config.at_connection_string + '?charset=utf8',
            echo=False)
        self._redis_helper = redis_helper
        self.meta = MetaData(self.__engine)
        self.event_table = Table('event', self.meta, autoload=True)

        self.impact_table = Table('impact', self.meta,
                                  Column('Event_ID', None,
                                         ForeignKey('event.Event_ID')),
                                  Column('TCObjectRef_ID', None, ForeignKey(
                                      'tcobjectref.TCObjectRef_ID')),
                                  autoload=True)

        self.tcobjectref_table = Table('tcobjectref', self.meta, autoload=True)

        self.impactbroadcast_table = \
            Table('impactbroadcast', self.meta,
                  Column('Impact_ID', None,
                         ForeignKey('impact.Impact_ID')),
                  Column('MsgMedia_ID', None,
                         ForeignKey('msgmedia.MsgMedia_ID')),
                  autoload=True)

        self.msgmedia_table = Table('msgmedia', self.meta, autoload=True)

        self.label_impact_id = 'impact_id'
        self.label_impact_state = 'impact_state'
        self.label_publication_start_date = 'publication_start_date'
        self.label_publication_end_date = 'publication_end_date'
        self.label_application_start_date = 'application_start_date'
        self.label_application_end_date = 'application_end_date'
        self.label_application_daily_start_hour =\
            'application_daily_start_hour'
        self.label_application_daily_end_hour =\
            'application_daily_end_hour'
        self.label_active_days = 'active_days'
        self.label_object_external_code = 'object_external_code'
        self.label_object_type = 'object_type'
        self.label_title = 'title'
        self.label_message = 'message'
        self.label_message_lang = 'lang'

        self.last_exec_file_name = './last_exec_time.txt'
        self.datetime_format = '%Y-%m-%d %H:%M:%S'
        self._collections = {"StopPoint": "stop_point",
                             "VehicleJourney": "vehicle_journey",
                             "Line": "line",
                             "Network": "network",
                             "StopArea": "stop_area"}

    def get_last_execution_time(self):
        if os.path.exists(self.last_exec_file_name):
            last_exec_file = open(self.last_exec_file_name, 'r')
            las_execution_time = last_exec_file.readline().rstrip('\n\r')
            return datetime.datetime.strptime(las_execution_time,
                                              self.datetime_format)
        else:
            return datetime.datetime.now()

    def set_last_execution_time(self, last_execution_time):
        last_exec_file = open(self.last_exec_file_name, 'w')
        last_exec_file.write(
            last_execution_time.strftime(self.datetime_format))

    def get_uri(self, externalcode, object_type):
        uri = None
        if object_type in self._collections.keys():
            self._redis_helper.set_prefix(self._collections[object_type])
            uri = self._redis_helper.get(externalcode)
        return uri

    def create_pertubation(self, message):
        pertubation = connectors.realtime_pb2.AtPerturbation()
        pertubation.uri = message.uri
        pertubation.object.object_uri = message.object.object_uri
        pertubation.object.object_type = message.object.object_type
        pertubation.start_application_date = \
            message.start_publication_date
        pertubation.end_application_date = \
            message.end_application_date
        pertubation.start_application_daily_hour = \
            message.start_application_daily_hour
        pertubation.end_application_daily_hour = \
            message.end_application_daily_hour
        pertubation.active_days = message.active_days
        return pertubation

    def get_status_message(self, status):
        if status.lower() == "information":
            return 0
        if status.lower() == "warning":
            return 1
        if status.lower() == "disrupt":
            return 2
        return None

    def set_message(self, result_proxy):
        """

        :param result_proxy:
        """
        last_impact_id = -1
        for row in result_proxy:
            try:
                current_uri = self.get_uri(
                    row[self.label_object_external_code],
                    row[self.label_object_type])
                if current_uri is None:
                    logging.getLogger('connector').warn(
                        "Object [%s] rejected : "
                        "No extcode and uri correspondance",
                        row[self.label_object_external_code])
                else:
                    if last_impact_id != row[self.label_impact_id]:
                        last_impact_id = row[self.label_impact_id]
                        message = connectors.realtime_pb2.Message()
                        self.message_list.append(message)

                        message.uri = str(row[self.label_impact_id]) + '-' + \
                            row[self.label_message_lang]

                        message.start_publication_date = get_pos_time(
                            row[self.label_publication_start_date])

                        message.end_publication_date = get_pos_time(
                            row[self.label_publication_end_date])
                        message.start_application_date = get_pos_time(
                            row[self.label_application_start_date])
                        message.end_application_date = get_pos_time(
                            row[self.label_application_end_date])
                        # number of seconds since midnigth
                        message.start_application_daily_hour = \
                            get_datetime_to_second(
                                row[self.label_application_daily_start_hour])
                        message.end_application_daily_hour = \
                            get_datetime_to_second(
                                row[self.label_application_daily_end_hour])
                        message.active_days = \
                            int_to_bitset(row[self.label_active_days]) + '1'

                        message.object.object_uri = current_uri

                        message.object.object_type = \
                            get_navitia_type(row[self.label_object_type])

                        message.message_status = self.get_status_message(
                            row[self.label_impact_state])

                    localized_message = message.localized_messages.add()
                    localized_message.language = row[self.label_message_lang]
                    localized_message.body = row[self.label_message]
                    localized_message.title = row[self.label_title]

                    if row[self.label_impact_state] == 'Disrupt':
                        self.perturbation_list.append(
                            self.create_pertubation(message))

                    print str(
                        row[self.label_active_days]) + ' - ' +\
                        message.active_days
            except google.protobuf.message.DecodeError as e:
                logging.getLogger('connector').warn(
                    "message is not a valid "
                    "protobuf task: %s", str(e))

    def set_request(self):
        return select([self.event_table.c.Event_ID,
                       self.impact_table.c.Impact_ID
                       .label(self.label_impact_id),
                       self.impact_table.c.Impact_State
                       .label(self.label_impact_state),
                       self.event_table.c.Event_PublicationStartDate
                       .label(self.label_publication_start_date),
                       self.event_table.c.Event_PublicationEndDate
                       .label(self.label_publication_end_date),
                       self.impact_table.c.Impact_EventStartDate
                       .label(self.label_application_start_date),
                       self.impact_table.c.Impact_EventEndDate
                       .label(self.label_application_end_date),
                       self.impact_table.c.Impact_DailyStartDate
                       .label(self.label_application_daily_start_hour),
                       self.impact_table.c.Impact_DailyEndDate
                       .label(self.label_application_daily_end_hour),
                       self.impact_table.c.Impact_ActiveDays
                       .label(self.label_active_days),
                       self.tcobjectref_table.c.TCObjectCodeExt
                       .label(self.label_object_external_code),
                       self.tcobjectref_table.c.TCObjectType
                       .label(self.label_object_type),
                       self.impactbroadcast_table.c.Impact_Title
                       .label(self.label_title),
                       self.impactbroadcast_table.c.Impact_Msg
                       .label(self.label_message),
                       self.msgmedia_table.c.MsgMedia_Lang
                       .label(self.label_message_lang)],
                      # clause where
                      and_(self.msgmedia_table.c.MsgMedia_Media == bindparam(
                          'media_media'),
                          self.event_table.c.Event_PublicationEndDate
                          >= bindparam('event_publicationenddate'),
                          or_(self.event_table.c.Event_CloseDate == None,
                              self.event_table.c.Event_CloseDate > bindparam(
                                  'event_closedate'),
                              self.impact_table.c
                              .Impact_SelfModificationDate == None,
                              self.impact_table.c
                              .Impact_SelfModificationDate > bindparam(
                                  'impact_modification_date'),
                              self.impact_table.c
                              .Impact_ChildrenModificationDate == None,
                              self.impact_table.c
                              .Impact_ChildrenModificationDate > bindparam(
                                  'impact_modification_date'))
                      ),
                      # jointure
                      from_obj=[self.event_table.join(self.impact_table).join(
                          self.tcobjectref_table).join(
                          self.impactbroadcast_table).join(
                          self.msgmedia_table)]
                      ).order_by(self.impact_table.c.Impact_ID)

    # impact_SelfModificationDate, impact_ChildrenModificationDate

    def execute(self):
        logger = logging.getLogger('connector')
        conn = None
        try:
            conn = self.__engine.connect()
        except exc.SQLAlchemyError as e:
            logger.exception('error durring connection')
        if conn is not None:
            result = None
            try:
                s = self.set_request()
                execution_time = datetime.datetime.now()
                # read last execution time
                last_execution_time = self.get_last_execution_time()
                result = \
                    conn.execute(s,
                                 media_media='Internet',
                                 event_publicationenddate=last_execution_time,
                                 #datetime.datetime(2013, 9, 01),
                                 event_closedate=datetime.datetime.now(),
                                 impact_modification_date=last_execution_time)
                # save execution time
                self.set_last_execution_time(execution_time)
            except exc.SQLAlchemyError as e:
                logger.exception('error during request')
            if result is not None:
                self.set_message(result)
                result.close()
