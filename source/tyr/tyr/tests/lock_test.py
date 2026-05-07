import logging
import pytest
from unittest.mock import patch, MagicMock, PropertyMock
from celery.exceptions import Retry, MaxRetriesExceededError
from redis.exceptions import ConnectionError
from tyr.binarisation import Lock


# =====================================================
# Helpers
# =====================================================


def make_mock_job(instance_name='test-instance', state='running'):
    """Create a mock Job with a mock Instance."""
    job = MagicMock()
    job.instance.name = instance_name
    job.state = state
    return job


def make_mock_task(max_retries_exceeded=False):
    """
    Create a mock Celery task.
    If max_retries_exceeded=True, task.retry() raises MaxRetriesExceededError.
    Otherwise, task.retry() raises Retry (normal Celery retry behavior).
    """
    task = MagicMock()
    if max_retries_exceeded:
        task.retry.side_effect = MaxRetriesExceededError()
    else:
        task.retry.side_effect = Retry()
    task.MaxRetriesExceededError = MaxRetriesExceededError
    return task


def make_decorated_function(lock_timeout=5400):
    """
    Create a simple function decorated with @Lock and return it
    along with a tracker to verify if the inner function was called.
    """
    tracker = MagicMock()

    @Lock(timeout=lock_timeout)
    def my_task(self, some_arg, job_id=None):
        tracker(some_arg, job_id)
        return 'task_result'

    return my_task, tracker


# =====================================================
# Patches applied to all tests
# =====================================================


@pytest.fixture(autouse=True)
def mock_dependencies():
    """Mock all external dependencies of Lock."""
    with patch('tyr.binarisation.models') as mock_models, patch('tyr.binarisation.redis') as mock_redis, patch(
        'tyr.binarisation.get_instance_logger'
    ) as mock_get_logger:
        mock_get_logger.return_value = logging.getLogger('test_lock')

        yield {
            'models': mock_models,
            'redis': mock_redis,
            'get_logger': mock_get_logger,
        }


# =====================================================
# Test 1: Lock acquired → function executes normally
# =====================================================


def test_lock_acquired_function_executes(mock_dependencies):
    """When the lock is acquired, the wrapped function should execute and return its result."""
    job = make_mock_job()
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_lock = MagicMock()
    mock_lock.acquire.return_value = True
    mock_dependencies['redis'].lock.return_value = mock_lock

    task = MagicMock()  # No retry needed
    decorated_fn, tracker = make_decorated_function(lock_timeout=5400)

    result = decorated_fn(task, 'arg1', job_id=42)

    assert result == 'task_result'
    tracker.assert_called_once_with('arg1', 42)
    mock_dependencies['redis'].lock.assert_called_once_with('tyr.lock|test-instance', timeout=5400)
    mock_lock.acquire.assert_called_once_with(blocking=False)
    task.retry.assert_not_called()


# =====================================================
# Test 2: Lock NOT acquired → Celery retry
# =====================================================


def test_lock_not_acquired_retries(mock_dependencies):
    """When the lock is NOT acquired, task.retry() should be called with countdown=300."""
    job = make_mock_job()
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_lock = MagicMock()
    mock_lock.acquire.return_value = False
    mock_dependencies['redis'].lock.return_value = mock_lock

    task = make_mock_task(max_retries_exceeded=False)
    decorated_fn, tracker = make_decorated_function(lock_timeout=5400)

    with pytest.raises(Retry):
        decorated_fn(task, 'arg1', job_id=42)

    tracker.assert_not_called()  # Function should NOT have been called
    task.retry.assert_called_once()
    call_kwargs = task.retry.call_args
    assert call_kwargs[1]['countdown'] == 300


# =====================================================
# Test 3: Lock NOT acquired + max retries exceeded
#          → job.state = 'failed'
# =====================================================


def test_lock_not_acquired_max_retries_sets_job_failed(mock_dependencies):
    """
    When the lock is NOT acquired and max retries are exceeded,
    the job state should be set to 'failed'.
    """
    job = make_mock_job(state='running')
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_lock = MagicMock()
    mock_lock.acquire.return_value = False
    mock_dependencies['redis'].lock.return_value = mock_lock

    task = make_mock_task(max_retries_exceeded=True)
    decorated_fn, tracker = make_decorated_function(lock_timeout=5400)

    with pytest.raises(MaxRetriesExceededError):
        decorated_fn(task, 'arg1', job_id=42)

        # Job should be marked as 'failed'
    assert job.state == 'failed'
    mock_dependencies['models'].db.session.commit.assert_called()
    tracker.assert_not_called()


# =====================================================
# Test 4: Redis ConnectionError → retry with countdown=10
# =====================================================


def test_redis_connection_error_retries(mock_dependencies):
    """When Redis raises ConnectionError, task should retry in 10 seconds."""
    job = make_mock_job()
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_dependencies['redis'].lock.side_effect = ConnectionError("Redis down")

    task = make_mock_task(max_retries_exceeded=False)
    decorated_fn, tracker = make_decorated_function()

    with pytest.raises(Retry):
        decorated_fn(task, 'arg1', job_id=42)

    tracker.assert_not_called()
    task.retry.assert_called_once()
    call_kwargs = task.retry.call_args
    assert call_kwargs[1]['countdown'] == 10
    assert call_kwargs[1]['max_retries'] == 10


# =====================================================
# Test 5: Redis ConnectionError + max retries exceeded
#          → job.state = 'failed'
# =====================================================


def test_redis_connection_error_max_retries_sets_job_failed(mock_dependencies):
    """
    When Redis raises ConnectionError and max retries are exceeded,
    the job state should be set to 'failed'.
    """
    job = make_mock_job(state='running')
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_dependencies['redis'].lock.side_effect = ConnectionError("Redis down")

    task = make_mock_task(max_retries_exceeded=True)
    decorated_fn, tracker = make_decorated_function()

    with pytest.raises(MaxRetriesExceededError):
        decorated_fn(task, 'arg1', job_id=42)

    assert job.state == 'failed'
    mock_dependencies['models'].db.session.commit.assert_called()


# =====================================================
# Test 6: max_retries adapté au lock timeout
# =====================================================


def test_max_retries_adapted_to_lock_timeout(mock_dependencies):
    """
    max_retries should be calculated as max(10, timeout // countdown + 2).
    For timeout=5400 and countdown=300: max_retries = max(10, 5400//300 + 2) = 20.
    """
    job = make_mock_job()
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_lock = MagicMock()
    mock_lock.acquire.return_value = False
    mock_dependencies['redis'].lock.return_value = mock_lock

    task = make_mock_task(max_retries_exceeded=False)
    # timeout = 90 * 60 = 5400 → max_retries = max(10, 5400 // 300 + 2) = 20
    decorated_fn, tracker = make_decorated_function(lock_timeout=5400)

    with pytest.raises(Retry):
        decorated_fn(task, 'arg1', job_id=42)

    call_kwargs = task.retry.call_args
    expected_max_retries = max(10, 5400 // 300 + 2)  # = 20
    assert call_kwargs[1]['max_retries'] == expected_max_retries


def test_max_retries_minimum_10_for_short_timeout(mock_dependencies):
    """
    Even with a short timeout, max_retries should be at least 10.
    For timeout=600 and countdown=300: max(10, 600//300 + 2) = max(10, 4) = 10.
    """
    job = make_mock_job()
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_lock = MagicMock()
    mock_lock.acquire.return_value = False
    mock_dependencies['redis'].lock.return_value = mock_lock

    task = make_mock_task(max_retries_exceeded=False)
    # timeout = 10 * 60 = 600 → max_retries = max(10, 600 // 300 + 2) = max(10, 4) = 10
    decorated_fn, tracker = make_decorated_function(lock_timeout=600)

    with pytest.raises(Retry):
        decorated_fn(task, 'arg1', job_id=42)

    call_kwargs = task.retry.call_args
    assert call_kwargs[1]['max_retries'] == 10


# =====================================================
# Test 7: Lock released after function execution
# =====================================================


def test_lock_released_after_execution(mock_dependencies):
    """The lock should be released after the wrapped function completes."""
    job = make_mock_job()
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_lock = MagicMock()
    mock_lock.acquire.return_value = True
    mock_dependencies['redis'].lock.return_value = mock_lock

    task = MagicMock()
    decorated_fn, tracker = make_decorated_function()

    decorated_fn(task, 'arg1', job_id=42)

    # lock_release is called via retrying.Retrying, which calls lock.release()
    mock_lock.release.assert_called()


# =====================================================
# Test 8: Lock released even if function raises
# =====================================================


def test_lock_released_on_function_exception(mock_dependencies):
    """The lock should be released even if the wrapped function raises an exception."""
    job = make_mock_job()
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_lock = MagicMock()
    mock_lock.acquire.return_value = True
    mock_dependencies['redis'].lock.return_value = mock_lock

    task = MagicMock()

    @Lock(timeout=5400)
    def failing_task(self, job_id=None):
        raise ValueError("task failed")

    with pytest.raises(ValueError, match="task failed"):
        failing_task(task, job_id=42)

    mock_lock.release.assert_called()


# =====================================================
# Test 9: job_id extrait des kwargs
# =====================================================


def test_job_id_from_kwargs(mock_dependencies):
    """Lock should correctly extract job_id from keyword arguments."""
    job = make_mock_job()
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_lock = MagicMock()
    mock_lock.acquire.return_value = True
    mock_dependencies['redis'].lock.return_value = mock_lock

    task = MagicMock()
    decorated_fn, tracker = make_decorated_function()

    decorated_fn(task, 'arg1', job_id=99)

    mock_dependencies['models'].Job.query.get.assert_called_once_with(99)


# =====================================================
# Test 10: Lock key uses instance name
# =====================================================


def test_lock_key_uses_instance_name(mock_dependencies):
    """The Redis lock key should include the instance name."""
    job = make_mock_job(instance_name='fr-idf')
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_lock = MagicMock()
    mock_lock.acquire.return_value = True
    mock_dependencies['redis'].lock.return_value = mock_lock

    task = MagicMock()
    decorated_fn, tracker = make_decorated_function(lock_timeout=3600)

    decorated_fn(task, 'arg1', job_id=42)

    mock_dependencies['redis'].lock.assert_called_once_with('tyr.lock|fr-idf', timeout=3600)


# =====================================================
# Test 11: Concurrent jobs on same instance → retry
# =====================================================


def test_concurrent_jobs_same_instance(mock_dependencies):
    """
    Simulates two concurrent jobs on the same instance.
    The second job should retry because the lock is already held.
    """
    job = make_mock_job(instance_name='fr-idf')
    mock_dependencies['models'].Job.query.get.return_value = job

    mock_lock = MagicMock()
    # First call: lock acquired. Second call: lock NOT acquired.
    mock_lock.acquire.side_effect = [True, False]
    mock_dependencies['redis'].lock.return_value = mock_lock

    task1 = MagicMock()
    task2 = make_mock_task(max_retries_exceeded=False)

    decorated_fn, tracker = make_decorated_function()

    # First job succeeds
    result = decorated_fn(task1, 'arg1', job_id=1)
    assert result == 'task_result'

    # Second job retries
    with pytest.raises(Retry):
        decorated_fn(task2, 'arg2', job_id=2)

    task2.retry.assert_called_once()
