# Metrics in Navitia

## Kraken

Kraken exposes its metrics by using the [prometheus interface](https://prometheus.io/docs/concepts/data_model/).
All kraken's metrics have a label `coverage` associated to them that corresponds to the coverage name.


| metric                               | Type      | description                                                                                          | labels                                   |
|--------------------------------------|-----------|------------------------------------------------------------------------------------------------------|------------------------------------------|
| kraken_request_duration_seconds      | Histogram | Duration of requests                                                                                 | api: the name of the kraken api measured |
| kraken_request_in_flight             | Gauge     | Number of request currently being handled                                                            |                                          |
| kraken_data_loading_duration_seconds | Histogram | Duration of data loading from data.nav.lz4                                                           |                                          |
| kraken_data_cloning_duration_seconds | Histogram | Duration of data cloning when applying disruption or realtime                                       |                                          |
| kraken_handle_rt_duration_seconds    | Histogram | Duration of disruption/realtime handling from the start of cloning to the end of all computations |                                          |
|                                      |           |                                                                                                      |                                          |
