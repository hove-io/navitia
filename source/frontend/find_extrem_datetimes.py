from datetime import datetime, timedelta
from operator import attrgetter
def extremes(resp, request): 
    before = None
    after = None

    if resp.planner and len(resp.planner.journey) > 0:
        asap_journey = min(resp.planner.journey, key=attrgetter('arrival_date_time'))
        query_args = ""
        for key, value in request.iteritems():
            if key != "datetime" and key != "clockwise":
                query_args += key + "=" +str(value) + "&"

        if asap_journey.arrival_date_time and asap_journey.departure_date_time:
            minute = timedelta(minutes = 1)
            datetime_after = datetime.strptime(asap_journey.departure_date_time, "%Y%m%dT%H%M%S") + minute
            before = query_args + "clockwise=true&datetime="+datetime_after.strftime("%Y%m%dT%H%M%S")
    
            datetime_before = datetime.strptime(asap_journey.arrival_date_time, "%Y%m%dT%H%M%S") - minute
            after = query_args +"clockwise=false&datetime="+datetime_before.strftime("%Y%m%dT%H%M%S")

    return (before,after)
