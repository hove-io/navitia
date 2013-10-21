# coding=utf-8
import type_pb2
import response_pb2
from instance_manager import NavitiaManager

class qualifier:
    def get_rabattement_duration(self,journey):
        sections = getattr(journey, "sections")
        current_duration = 0
        if sections[0].type == response_pb2.STREET_NETWORK:
            current_duration = sections[0].duration
        if sections[-1].type == response_pb2.STREET_NETWORK:
            current_duration = current_duration + sections[-1].duration
        return current_duration

    def set_streetnetwork_params(self,params,req):
        req.journeys.streetnetwork_params.origin_mode = params[0]["mode"]
        req.journeys.streetnetwork_params.destination_mode = params[0]["mode"]
        req.journeys.streetnetwork_params.bike_speed =  params[0]["bike_speed"]
        req.journeys.streetnetwork_params.bike_distance = params[0]["bike_distance"]
        req.journeys.streetnetwork_params.car_speed =  params[0]["car_speed"]
        req.journeys.streetnetwork_params.car_distance = params[0]["car_distance"]
        req.journeys.streetnetwork_params.walking_speed =  params[0]["walking_speed"]
        req.journeys.streetnetwork_params.walking_distance = params[0]["walking_distance"]
        del params[0]

    def is_car(self, section):
        to_return = False
        if section.type == response_pb2.STREET_NETWORK:
            if section.street_network.mode == response_pb2.Car:
                to_return = True
        return to_return

    def qualifier_one(self, req, region):
        params = []
        params.append({"mode":"walking","walking_speed":14*0.277777778, "walking_distance":1500,
                       "bike_speed":req.journeys.streetnetwork_params.bike_speed,
                       "bike_distance":req.journeys.streetnetwork_params.bike_distance,
                        "car_speed":req.journeys.streetnetwork_params.car_speed,
                        "car_distance":req.journeys.streetnetwork_params.car_distance})
        params.append({"mode":"bike","bike_speed":14*0.277777778, "bike_distance":4000,
                        "walking_speed":req.journeys.streetnetwork_params.walking_speed,
                        "walking_distance":req.journeys.streetnetwork_params.walking_distance,
                        "car_speed":req.journeys.streetnetwork_params.car_speed,
                        "car_distance":req.journeys.streetnetwork_params.car_distance})
        params.append({"mode":"car","car_speed": 20*0.277777778,"car_distance" :20*1000,
                        "walking_speed":req.journeys.streetnetwork_params.walking_speed,
                        "walking_distance":req.journeys.streetnetwork_params.walking_distance,
                        "bike_speed":req.journeys.streetnetwork_params.bike_speed,
                        "bike_distance":req.journeys.streetnetwork_params.bike_distance})
        trajet = {"rapid" : -1, "rapid_plus" : -1,
                      "comfort" : -1, "healthy" : -1}

        self.set_streetnetwork_params(params, req)
        resp = NavitiaManager().send_and_receive(req, region)
        while len(params) > 0 and resp.response_type != response_pb2.ITINERARY_FOUND:
            self.set_streetnetwork_params(params, req)
            resp = NavitiaManager().send_and_receive(req, region)

        if resp.response_type == response_pb2.ITINERARY_FOUND:
            journeys = getattr(resp, "journeys")

            if len(journeys)>0:
                trajet["rapid"] = 0
            index = -1
            for journey in journeys:
                index = index + 1
                if journeys[trajet["rapid"]].arrival_date_time > journey.arrival_date_time :
                    trajet["rapid"] = index

            if trajet["rapid"] > -1 :
                index = -1
                for journey in journeys :
                    index = index + 1
                    if journey.duration > 1.2*journeys[trajet["rapid"]].duration and \
                        journey.nb_transfers < journeys[trajet["rapid"]].nb_transfers :
                        trajet["rapid_plus"] = index

                if trajet["rapid_plus"] > -1 :
                    trajet["rapid"] = trajet["rapid_plus"]
                index = -1
                rapid_duration = self.get_rabattement_duration(journeys[trajet["rapid"]])
                for journey in journeys :
                    index = index + 1
                    if journey.duration > 1.5*journeys[trajet["rapid"]].duration :
                        current_duration = self.get_rabattement_duration(journey)
                        sections = getattr(journey, "sections")
                        if (not self.is_car(sections[0])) and \
                            (not self.is_car(sections[-1])) and\
                            current_duration > 0.1*journey.duration:
                            trajet["healthy"] = index
                        if journey.nb_transfers < journeys[trajet["rapid"]].nb_transfers or \
                            current_duration < rapid_duration:
                            trajet["comfort"] = index

            if trajet["rapid"] > -1 :
                journeys[trajet["rapid"]].type =  "rapid"
            if trajet["comfort"] > -1 :
                journeys[trajet["comfort"]].type = "comfort"
            if trajet["healthy"] > -1 :
                journeys[trajet["healthy"]].type = "healthy"

        return resp