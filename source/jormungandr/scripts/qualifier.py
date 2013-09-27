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
        trajet = {"presse" : -1, "presse_plus" : -1,
                      "confort" : -1, "sante" : -1}

        self.set_streetnetwork_params(params, req)
        resp = NavitiaManager().send_and_receive(req, region)
        while len(params) > 0 and resp.response_type != response_pb2.ITINERARY_FOUND:
            self.set_streetnetwork_params(params, req)
            resp = NavitiaManager().send_and_receive(req, region)

        if resp.response_type == response_pb2.ITINERARY_FOUND:
            journeys = getattr(resp, "journeys")

            if len(journeys)>0:
                trajet["presse"] = 0
            index = -1
            for journey in journeys:
                index = index + 1
                if journeys[trajet["presse"]].arrival_date_time > journey.arrival_date_time :
                    trajet["presse"] = index

            if trajet["presse"] > -1 :
                index = -1
                for journey in journeys :
                    index = index + 1
                    if journey.duration > 1.2*journeys[trajet["presse"]].duration and \
                        journey.nb_transfers < journeys[trajet["presse"]].nb_transfers :
                        trajet["presse_plus"] = index

                if trajet["presse_plus"] > -1 :
                    trajet["presse"] = trajet["presse_plus"]
                index = -1
                presse_duration = self.get_rabattement_duration(journeys[trajet["presse"]])
                for journey in journeys :
                    index = index + 1
                    if journey.duration > 1.5*journeys[trajet["presse"]].duration :
                        current_duration = self.get_rabattement_duration(journey)
                        sections = getattr(journey, "sections")
                        if (not self.is_car(sections[0])) and \
                            (not self.is_car(sections[-1])) and\
                            current_duration > 0.1*journey.duration:
                            trajet["sante"] = index
                        if journey.nb_transfers < journeys[trajet["presse"]].nb_transfers or \
                            current_duration < presse_duration:
                            trajet["confort"] = index

            if trajet["presse"] > -1 :
                journeys[trajet["presse"]].type =  "presse"
            if trajet["confort"] > -1 :
                journeys[trajet["confort"]].type = "confort"
            if trajet["sante"] > -1 :
                journeys[trajet["sante"]].type = "sante"

        return resp