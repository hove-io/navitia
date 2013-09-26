#encoding: utf-8
import logging
import pymssql
from sqlalchemy import Column, Table, MetaData, select, create_engine, ForeignKey
import sqlalchemy

#
# SELECT "
#             "event.event_id AS event_id, " //0
#             "impact.impact_id AS impact_id, " //1
#             "convert(varchar, event.event_publicationstartdate, 121) AS  publication_start_date, "//2
#             "convert(varchar, event.event_publicationenddate, 121) AS  publication_end_date, "//3
#             "convert(varchar, impact.impact_eventstartdate, 121) AS application_start_date, "//4
#             "convert(varchar, impact.impact_eventenddate, 121) AS application_end_date, "//5
#             "convert(varchar, impact.impact_dailystartdate, 108) AS application_daily_start_hour, "//6
#             "convert(varchar, impact.impact_dailyenddate, 108) AS application_daily_end_hour, "//7
#             "impact.impact_activedays AS active_days, "//8
#             "tcobjectref.tcobjectcodeext AS object_external_code, "//9
#             "tcobjectref.tcobjecttype AS object_type, "//10
#             "impactBroadcast.impact_title AS title, "//11
#             "ImpactBroadcast.impact_msg AS message "//12
#         "FROM "
#             "event "
#             "INNER JOIN impact ON (event.event_id = impact.event_id) "
#             "INNER JOIN tcobjectref ON (impact.tcobjectref_id = tcobjectref.tcobjectref_id) "
#             "INNER JOIN impactbroadcast ON (impactbroadcast.Impact_ID = impact.Impact_ID) "
#             "INNER JOIN msgmedia ON (impactbroadcast.msgmedia_id = msgmedia.msgmedia_id) "
#         "WHERE "
#             "event.event_publicationenddate >= CONVERT(DATETIME, :date_pub, 126) "
#             "AND (event.event_closedate IS NULL OR event.event_closedate > CONVERT(DATETIME, :date_clo, 126)) "
#             "AND msgmedia.msgmedia_lang = :media_lang "
#             "AND msgmedia.msgmedia_media = :media_media"
#     );


class AtRealtimeReader(object):
    """
    Classe responsable de la lecture en base de donnée des événements
    temps réel.
    """

    def __init__(self):
        self.username = 'sa'
        self.password = '159can87*'
        self.server = '10.2.0.16'
        self.port = '1433'
        self.database = 'at_lyon2sim'
        #dialect+driver://user:password@host/dbname[?key=value..]
        connection_string = 'mssql+pymssql://%s:%s@%s/%s' % (
                self.username,
                self.password,
                self.server,
                self.database
        )

        self.__engine = create_engine(
            connection_string, echo=False)

        self.meta = MetaData(self.__engine)
        self.event_table = Table('event', self.meta, autoload=True)

        self.impact_table = Table('impact', self.meta,
        Column('Event_ID', None, ForeignKey('event.Event_ID')),
        Column('TCObjectRef_ID', None, ForeignKey('tcobjectref.TCObjectRef_ID')),
        autoload=True)

        self.tcobjectref_table = Table('tcobjectref', self.meta, autoload=True)

        self.impactbroadcast_table = Table('impactbroadcast', self.meta,
        Column('Impact_ID', None, ForeignKey('impact.Impact_ID')),
        Column('MsgMedia_ID', None, ForeignKey('msgmedia.MsgMedia_ID')),
        autoload=True)

        self.msgmedia_table = Table('msgmedia', self.meta, autoload=True)

    def run(self):
        logger = logging.getLogger('connector')
        conn = None
        try:
            conn = self.__engine.connect()
        except sqlalchemy.exc.SQLAlchemyError, e:
            logger.exception('error durring connection')
        if conn is not None:
            result = None
            try:
                s = select([self.event_table.c.Event_ID,
                            self.impact_table.c.Impact_ID,
                            self.event_table.c.Event_PublicationStartDate,
                            self.event_table.c.Event_PublicationEndDate,
                            self.impact_table.c.Impact_EventStartDate,
                            self.impact_table.c.Impact_EventEndDate,
                            self.impact_table.c.Impact_DailyStartDate,
                            self.impact_table.c.Impact_DailyEndDate,
                            self.impact_table.c.Impact_ActiveDays,
                            self.tcobjectref_table.c.TCObjectCodeExt,
                            self.tcobjectref_table.c.TCObjectType,
                            self.impactbroadcast_table.c.Impact_Title,
                            self.impactbroadcast_table.c.Impact_Msg
                            ],
                    from_obj=[self.event_table.join(self.impact_table).join(
                        self.tcobjectref_table).join(
                        self.impactbroadcast_table).join(self
                    .msgmedia_table)],
                    self.msgmedia_table.c.MsgMedia_Lang = :media_lang)
                result = conn.execute(s)
            except sqlalchemy.exc.SQLAlchemyError, e:
                logger.exception('error durring request')
            if result is not None:
                for row in result:
                    print row
            result.close();



if __name__ == '__main__':
    atrealtimereader = AtRealtimeReader()
    atrealtimereader.run()
