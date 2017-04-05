from jormungandr.street_network.street_network import DirectPathType
from jormungandr.utils import PeriodExtremity, SectionSorter
from navitiacommon import response_pb2
from Exceptions import *
import uuid
import copy


def _create_crowfly(pt_journey, crowfly_origin, crowfly_destination, begin, end):
    section = response_pb2.Section()
    section.type = response_pb2.CROW_FLY
    section.origin.CopyFrom(crowfly_origin)
    section.destination.CopyFrom(crowfly_destination)
    section.duration = end - begin
    pt_journey.durations.walking += section.duration
    pt_journey.durations.total += section.duration
    pt_journey.duration += section.duration
    section.begin_date_time = begin
    section.end_date_time = end
    section.street_network.mode = response_pb2.Walking
    section.id = unicode(uuid.uuid4())
    return section


def _is_crowfly_needed(uri, fallback_durations, crowfly_sps, fallback_direct_path):
    # Unable to project
    f = fallback_durations.get(uri, None)
    is_unknown_projection = (f.status == response_pb2.unknown if f else False)

    is_crowfly_sp = (uri in crowfly_sps)

    # At this point, theoretically, fallback_dp should be found since the isochrone has already given a
    # valid value BUT, in some cases(due to the bad projection, etc), fallback_dp may not exist even
    # though the isochrone said "OK". In these cases, we create a crowfly section
    is_empty_fallback_dp = (not fallback_direct_path or not fallback_direct_path.journeys)
    return is_unknown_projection or is_crowfly_sp or is_empty_fallback_dp


def _align_fallback_direct_path_datetime(fallback_direct_path, fallback_extremity):
    """
    :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
    in this function, we retrieve from fallback_direct_path_pool the direct path regarding to the given
    parameters(mode, orig_uri, etc...) then we recompute the datetimes of the found direct path,
    since the request datetime might no longer be the same (but we consider the same fallback duration).
    Nota: this implementation is connector-specific (so shouldn't be here)
    """

    # align datetimes to requested ones (as we consider fallback duration are the same no matter when)
    fallback_copy = copy.deepcopy(fallback_direct_path)
    journey = fallback_copy.journeys[0]
    datetime, represents_start_fallback = fallback_extremity
    if represents_start_fallback:
        journey.departure_date_time = datetime
        journey.arrival_date_time = datetime + journey.duration
    else:
        journey.departure_date_time = datetime - journey.duration
        journey.arrival_date_time = datetime

    delta = journey.departure_date_time - journey.sections[0].begin_date_time
    if delta != 0:
        for s in journey.sections:
            s.begin_date_time += delta
            s.end_date_time += delta
    return fallback_copy


def _rename_journey_sections_ids(start_idx, sections):
    for s in sections:
        s.id = "dp_section_{}".format(start_idx)
        start_idx += 1


def _extend_pt_sections_with_direct_path(pt_journey, dp_journey):
    if getattr(dp_journey, 'journeys', []) and hasattr(dp_journey.journeys[0], 'sections'):
        _rename_journey_sections_ids(len(pt_journey.sections), dp_journey.journeys[0].sections)
        pt_journey.sections.extend(dp_journey.journeys[0].sections)


def _extend_journey(pt_journey, fallback_dp, fallback_extremity):
    '''
    :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
    '''
    alinged_fallback = _align_fallback_direct_path_datetime(fallback_dp, fallback_extremity)
    pt_journey.duration += alinged_fallback.journeys[0].duration
    pt_journey.durations.total = pt_journey.duration
    pt_journey.durations.walking += alinged_fallback.journeys[0].durations.walking
    _extend_pt_sections_with_direct_path(pt_journey, alinged_fallback)


def _build_from(requested_orig_obj, pt_journeys, dep_mode, fallback_direct_path_pool, orig_accessible_by_crowfly,
                orig_fallback_durations_pool):

    accessibles_by_crowfly = orig_accessible_by_crowfly.wait_and_get()
    fallback_durations = orig_fallback_durations_pool.wait_and_get(dep_mode)

    for pt_journey in pt_journeys.journeys:
        pt_origin = pt_journey.sections[0].origin

        fallback_extremity = PeriodExtremity(pt_journey.departure_date_time, False)
        # extend the journey with the fallback routing path
        direct_path_type = DirectPathType.BEGINNING_FALLBACK
        fallback_dp = fallback_direct_path_pool.wait_and_get(requested_orig_obj, pt_origin,
                                                             dep_mode, fallback_extremity, direct_path_type)

        if requested_orig_obj.uri != pt_origin.uri:
            if pt_origin.uri in accessibles_by_crowfly.odt:
                pt_journey.sections[0].origin.CopyFrom(requested_orig_obj)
            elif _is_crowfly_needed(pt_origin.uri, fallback_durations, accessibles_by_crowfly.crowfly, fallback_dp):
                crowfly_departure_dt = pt_journey.departure_date_time - fallback_durations[pt_origin.uri].duration
                pt_journey.sections.extend([_create_crowfly(pt_journey, requested_orig_obj, pt_origin,
                                                            crowfly_departure_dt,
                                                            pt_journey.sections[0].begin_date_time)])
            else:
                # extend the journey with the fallback routing path
                _extend_journey(pt_journey, fallback_dp, fallback_extremity)

        pt_journey.sections.sort(SectionSorter())
        pt_journey.departure_date_time = pt_journey.sections[0].begin_date_time

    return pt_journeys


def _build_to(requested_dest_obj, pt_journeys, arr_mode, fallback_direct_path_pool, dest_accessible_by_crowfly,
              dest_fallback_durations_pool):

    accessibles_by_crowfly = dest_accessible_by_crowfly.wait_and_get()
    fallback_durations = dest_fallback_durations_pool.wait_and_get(arr_mode)

    for pt_journey in pt_journeys.journeys:
        pt_destination = pt_journey.sections[-1].destination
        last_section_end = pt_journey.sections[-1].end_date_time

        if requested_dest_obj.uri != pt_destination.uri:
            fallback_extremity = PeriodExtremity(pt_journey.arrival_date_time, True)
            # extend the journey with the fallback routing path
            direct_path_type = DirectPathType.ENDING_FALLBACK
            fallback_dp = fallback_direct_path_pool.wait_and_get(pt_destination, requested_dest_obj,
                                                                 arr_mode, fallback_extremity, direct_path_type)
            if pt_destination.uri in accessibles_by_crowfly.odt:
                pt_journey.sections[-1].destination.CopyFrom(requested_dest_obj)
            elif _is_crowfly_needed(pt_destination.uri, fallback_durations, accessibles_by_crowfly.crowfly, fallback_dp):
                crowfly_arrival_dt = pt_journey.arrival_date_time + fallback_durations[pt_destination.uri].duration
                pt_journey.sections.extend([_create_crowfly(pt_journey, pt_destination, requested_dest_obj,
                                                            last_section_end,
                                                            crowfly_arrival_dt)])
            else:
                _extend_journey(pt_journey, fallback_dp, fallback_extremity)

        pt_journey.sections.sort(SectionSorter())
        pt_journey.arrival_date_time = pt_journey.sections[-1].end_date_time
    return pt_journeys


def _clean_pt_journey_error_or_raise(pt_journeys, has_valid_direct_path_no_pt):
    if pt_journeys and pt_journeys.HasField(b"error"):
        if pt_journeys.error.id == response_pb2.Error.error_id.Value('no_solution') and has_valid_direct_path_no_pt:
            pt_journeys.ClearField(b"error")
        else:
            raise PtException(pt_journeys)


def get_max_fallback_duration(request, mode, dp_future):
    """
    By knowing the duration of direct path, we can limit the max duration for proximities by crowfly and fallback
    durations
    :param request:
    :param mode:
    :param dp_future:
    :return:  max_fallback_duration
    """
    # 30 minutes by default
    max_duration = request.get('max_{}_duration_to_pt'.format(mode), 1800)
    dp = dp_future.wait_and_get() if dp_future else None
    dp_duration = dp.journeys[0].durations.total if getattr(dp, 'journeys', None) else max_duration
    return min(max_duration, dp_duration)


def compute_fallback(requested_orig_obj, requested_dest_obj, pt_journey_pool, direct_path_pool,
                     orig_accessible_by_crowfly, dest_accessible_by_crowfly, request):
    has_valid_direct_path_no_pt = direct_path_pool.has_valid_direct_path_no_pt()
    for (dep_mode, arr_mode, pt_journey_f) in pt_journey_pool:
        pt_journeys = pt_journey_f.wait_and_get()
        print(dep_mode, arr_mode, "launching fallback")

        _clean_pt_journey_error_or_raise(pt_journeys, has_valid_direct_path_no_pt)

        if not getattr(pt_journeys, "journeys", None):
            continue

        accesible_crowfly = orig_accessible_by_crowfly.wait_and_get()
        orig_all_crowfly = accesible_crowfly.odt | accesible_crowfly.crowfly
        accesible_crowfly = dest_accessible_by_crowfly.wait_and_get()
        dest_all_crowfly = accesible_crowfly.odt | accesible_crowfly.crowfly

        for journey in pt_journeys.journeys:
            # from
            pt_orig = journey.sections[0].origin
            direct_path_type = DirectPathType.BEGINNING_FALLBACK
            fallback_extremity_dep = PeriodExtremity(journey.departure_date_time, False)
            if requested_orig_obj.uri != pt_orig.uri and pt_orig.uri not in orig_all_crowfly:
                direct_path_pool.add_async_request(requested_orig_obj, pt_orig, dep_mode, fallback_extremity_dep,
                                                   request, direct_path_type)

            # to
            pt_dest = journey.sections[-1].destination
            direct_path_type = DirectPathType.ENDING_FALLBACK
            fallback_extremity_arr = PeriodExtremity(journey.arrival_date_time, True)
            if requested_dest_obj.uri != pt_dest.uri and pt_dest.uri not in dest_all_crowfly:
                direct_path_pool.add_async_request(pt_dest, requested_dest_obj, arr_mode, fallback_extremity_arr,
                                                   request, direct_path_type)


def complete_pt_journey(requested_orig_obj, requested_dest_obj, pt_journey_pool_elem, direct_path_pool,
                        orig_accessible_by_crowfly, dest_accessible_by_crowfly,
                        orig_fallback_durations_pool, dest_fallback_durations_pool):
    pt_journeys = copy.deepcopy(pt_journey_pool_elem.pt_journey.wait_and_get())
    if not getattr(pt_journeys, "journeys", None):
        return pt_journeys
    dep_mode = pt_journey_pool_elem.dep_mode
    arr_mode = pt_journey_pool_elem.arr_mode

    pt_journeys = _build_from(requested_orig_obj, pt_journeys, dep_mode, direct_path_pool,
                              orig_accessible_by_crowfly, orig_fallback_durations_pool)

    pt_journeys = _build_to(requested_dest_obj, pt_journeys, arr_mode, direct_path_pool,
                            dest_accessible_by_crowfly, dest_fallback_durations_pool)

    return pt_journeys


def check_entry_point_or_raise(entry_point_obj, requested_uri):
    if not entry_point_obj:
        raise EntryPointException("The entry point: {} is not valid".format(requested_uri),
                                  response_pb2.Error.unknown_object)


def check_final_results_or_raise(final_results, orig_fallback_durations_pool, dest_fallback_durations_pool):
    if final_results:
        return
    orig_fallback_durations_is_empty = orig_fallback_durations_pool.is_empty()
    dest_fallback_durations_is_empty = dest_fallback_durations_pool.is_empty()
    if orig_fallback_durations_is_empty and dest_fallback_durations_is_empty:
        raise EntryPointException("no origin point nor destination point",
                                  response_pb2.Error.no_origin_nor_destination)
    if orig_fallback_durations_is_empty:
        raise EntryPointException("no origin point", response_pb2.Error.no_origin)
    if dest_fallback_durations_is_empty:
        raise EntryPointException("no destination point", response_pb2.Error.no_destination)
