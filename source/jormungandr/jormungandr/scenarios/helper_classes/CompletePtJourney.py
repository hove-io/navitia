import Future
from helper_utils import complete_pt_journey, compute_fallback


def wait_and_complete_pt_journey(requested_orig_obj, requested_dest_obj, pt_journey_pool, direct_path_pool,
                                 orig_accessible_by_crowfly, dest_accessible_by_crowfly,
                                 orig_fallback_durations_pool, dest_fallback_durations_pool,
                                 request):
    # launch fallback direct path asynchronously
    compute_fallback(requested_orig_obj, requested_dest_obj, pt_journey_pool, direct_path_pool,
                     orig_accessible_by_crowfly, dest_accessible_by_crowfly, request)

    futures = []
    for elem in pt_journey_pool:
        f = Future.create_future(complete_pt_journey, requested_orig_obj, requested_dest_obj,
                                 elem, direct_path_pool,
                                 orig_accessible_by_crowfly, dest_accessible_by_crowfly,
                                 orig_fallback_durations_pool, dest_fallback_durations_pool)
        futures.append(f)
    Future.wait_all(futures)
    return [f.wait_and_get() for f in futures if f.wait_and_get()]

