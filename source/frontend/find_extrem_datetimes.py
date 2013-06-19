from datetime import datetime, timedelta
from operator import attrgetter
import sys
def extremes(resp, request): 
    before = None
    after = None

    try:
        asap_journey = min([journey for journey in resp.journeys if journey.arrival_date_time != ''], key=attrgetter('arrival_date_time'))
    except:
        #print "Unexpected error in prev/next datetime:", sys.exc_info()[0]
        return (None, None)

    query_args = ""
    for key, value in request.iteritems():
        if key != "datetime" and key != "clockwise":
            if type(value) == type([]):
                for v in value:
                    query_args += key + "=" +str(v) + "&"
            else:
                query_args += key + "=" +str(value) + "&"
    
    if asap_journey.arrival_date_time and asap_journey.departure_date_time:
        minute = timedelta(minutes = 1)
        datetime_after = datetime.strptime(asap_journey.departure_date_time, "%Y%m%dT%H%M%S") + minute
        after = query_args + "clockwise=true&datetime="+datetime_after.strftime("%Y%m%dT%H%M%S")

        datetime_before = datetime.strptime(asap_journey.arrival_date_time, "%Y%m%dT%H%M%S") - minute
        before = query_args +"clockwise=false&datetime="+datetime_before.strftime("%Y%m%dT%H%M%S")

    return (before,after)
