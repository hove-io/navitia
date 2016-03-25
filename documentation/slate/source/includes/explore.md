<a name="explore"></a>Explore transport
=======================================

-   **[Coverage](#coverage)** : List of the region covered by navitia

| url | Result |
|----------------------------------------------|-------------------------------------|
| `get` /coverage                              | List of the areas covered by navitia|
| `get` /coverage/*region_id*                  | Information about a specific region |
| `get` /coverage/*region_id*/coords/*lon;lat* | Information about a specific region |

-   **[Public transportation objects](#pt-ref)** : List of the public transport
    objects of a region

| url | Result |
|---------------------------------------------------------|-------------------------------------|
| `get` /coverage/*region_id*/*collection_name*           | Collection of objects of a region   |
| `get` /coverage/region_id/collection_name/object_id     | Information about a specific region |
| `get` /coverage/*lon;lat*/*collection_name*             | Collection of objects of a region   |
| `get` /coverage/*lon;lat*/*collection_name*/*object_id* | Information about a specific region |

-   **[Places](#places)** and **[PT_objects](#pt-objects)** : Search in the datas

| url | Result |
|------------------------------------------------|-------------------------------------|
| `get` /coverage/places                         | List of geographical objects        |
| `get` /coverage/pt_objects                     | List of public transport objects    |


