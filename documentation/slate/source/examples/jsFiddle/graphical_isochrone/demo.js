var sandboxToken = 'a61f4cd8-3007-4557-9f4c-214969369cca';

// Isochron starting point
var from = [48.846905, 2.377097];

// Limit isochron duration (required, or may trigger timeout when there is more data)
var maxDuration = 2000;
var minDuration = 1000;

// Navitia query for this isochron
var url = 'https://api.navitia.io/v1/coverage/sandbox/isochrones?from='+from[1]+';'+from[0]+'&datetime=20160505T080000&max_duration='+maxDuration+'&min_duration='+minDuration;

// Call navitia api
$.ajax({
       type: 'GET',
       url: url,
       dataType: "json",
       headers: {
        Authorization: 'Basic ' + btoa(sandboxToken)
    	 },
       success : drawIsochron,
       error: function(xhr, textStatus, errorThrown) {
       alert('Error when trying to process isochron: "'+textStatus+'", "'+errorThrown+'"');
    }
});

var tiles = {
    url: 'http://www.toolserver.org/tiles/bw-mapnik/{z}/{x}/{y}.png',
    attrib: 'Navitia Isochron example © <a href="http://openstreetmap.org">OpenStreetMap</a> contributors'
};

// Create a drawable map using Leaflet
var map = L.map('map')
    .setView(from, 13)
    .addLayer(new L.TileLayer(tiles.url, {minZoom: 0, maxZoom: 16, attribution: tiles.attrib}));

// Add marker to show isochron starting point
L.marker(from).addTo(map);


function drawIsochron (result) {
$.each(result.isochrones, function (i, isochrone) {
	var polygon = isochrone.geojson;
  var myLayer = L.geoJson(polygon).addTo(map);
  var newBounds = myLayer.getBounds();
  map.fitBounds(newBounds);
});
}

