from datetime import datetime, timedelta
def extremes(resp, request) : 
    response = {"before" : "", "after":""}
    earlier = "22221101T010159"
    later = "20000101T000000"
    if resp.planner != None:
        if resp.planner != None:
            for journey in resp.planner.journey:
                if journey.departure_date_time < earlier:
                    earlier = journey.departure_date_time
                if journey.arrival_date_time > later:
                    later = journey.arrival_date_time

            
            tmp = ""
            for key, value in request.iteritems():
                if key != "datetime" and key != "clockwise":
                    tmp += key + "=" +str(value) + "&"

            minute = timedelta(minutes = 1)
            if earlier != "22221101T0101" :
                d = datetime.strptime(earlier, "%Y%m%dT%H%M%S")
                datetime_after = d + minute
                response["after"] = tmp + "clockwise=true&datetime="+datetime_after.strftime("%Y%m%dT%H%M%S")

            if later != "20000101T000000":
                d = datetime.strptime(later, "%Y%m%dT%H%M%S") 
                print minute
                print "later : " + d.strftime("%Y%m%dT%H%M%S")
                datetime_before = d - minute
                response["before"] = tmp +"clockwise=false&datetime="+datetime_before.strftime("%Y%m%dT%H%M%S")
    return response
