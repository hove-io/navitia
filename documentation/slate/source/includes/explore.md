<a name="explore"></a>![Explore transport](/images/search_places.png)Explore transport
=======================================

The Explore Transport feature lets you explore places, coordinates, bus stops, subway stations, etc. 
to navigate all the data available on the API (collection service). 

You can use these APIs (click on them for details):

-   **[Coverage](#coverage)** : List of the region covered by navitia

| url | Result |
|----------------------------------------------|-------------------------------------|
| `/coverage`                              | List of the areas covered by navitia|
| `/coverage/{region_id}`                  | Information about a specific region |
| `/coverage/{region_id}/coords/{lon;lat}` | Information about a specific region |

-   **[Public transportation objects](#pt-ref)** : List of the public transport
    objects of a region

| url | Result |
|---------------------------------------------------------|-------------------------------------|
| `/coverage/{region_id}/{collection_name}`           | Collection of objects of a region   |
| `/coverage/{region_id}/{collection_name}/{object_id}`     | Information about a specific region |
| `/coverage/{lon;lat}/{collection_name}`             | Collection of objects of a region   |
| `/coverage/{lon;lat}/{collection_name}/{object_id}` | Information about a specific region |

-   **[Places](#places)** and **[PT_objects](#pt-objects)** : Search in the datas using autocomplete input.

| url | Result |
|------------------------------------------------|-------------------------------------|
| `/coverage/places`                         | List of geographical objects        |
| `/coverage/pt_objects`                     | List of public transport objects    |


