var map;  

var depart_arrivee = {
    depart :  { lat : -1, lon : -1},
    arrivee : { lat : -1, lon : -1}
};

function aff_data(data) {
    planner = data.planner;
    map.removeAllPolylines();
    map.removeAllMarkers();
    var arraypolys = new Array();

    for(i=0;i<planner.feuilleroute.etapes.length;i++) {
        var marker;
        if(i==0) {
            marker = new mxn.Marker(new mxn.LatLonPoint(planner.feuilleroute.etapes[i].depart.lieu.geo.y, planner.feuilleroute.etapes[i].depart.lieu.geo.x));

            marker.setInfoBubble("Depart");


            map.addMarker(marker);


            marker = new mxn.Marker(new mxn.LatLonPoint(planner.feuilleroute.etapes[i].arrivee.lieu.geo.y, planner.feuilleroute.etapes[i].arrivee.lieu.geo.x));
            map.addMarker(marker);
            marker.openBubble();
            $("#details").html("");
            $("#details").append($('<br>'));
            $("#details").append("Depart à " +planner.feuilleroute.etapes[i].depart.date.heure+" le : "+planner.feuilleroute.etapes[i].depart.date.date+" de "+ planner.feuilleroute.etapes[i].depart.lieu.nom+" avec la ligne : " +planner.feuilleroute.etapes[i].mode.ligne);
            $("#details").append($('<br>'));
            $("#details").append(" arrivée à : "+planner.feuilleroute.etapes[i].arrivee.date.heure+" le : "+planner.feuilleroute.etapes[i].arrivee.date.date);
            $("#details").append($("<br>"));

        } else {
            marker = new mxn.Marker(new mxn.LatLonPoint(planner.feuilleroute.etapes[i].arrivee.lieu.geo.y, planner.feuilleroute.etapes[i].arrivee.lieu.geo.x));
            marker.setInfoBubble("Depart arrivee");


            map.addMarker(marker);
            $("#details").append("Depart à " +planner.feuilleroute.etapes[i].depart.date.heure+" le : "+planner.feuilleroute.etapes[i].depart.date.date+" de "+ planner.feuilleroute.etapes[i].depart.lieu.nom+" avec la ligne : " +planner.feuilleroute.etapes[i].mode.ligne);
            $("#details").append($('<br>'));
            $("#details").append(" arrivée à : "+planner.feuilleroute.etapes[i].arrivee.date.heure+" le : "+planner.feuilleroute.etapes[i].arrivee.date.date);
            $("#details").append($("<br>"));
        }

    }


    for(i=0;i<planner.itineraire.trajets.length;i++) {

        for(j=0;j<planner.itineraire.trajets[i].pas.length;j++) {
            arraypolys.push(new mxn.LatLonPoint(planner.itineraire.trajets[i].pas[j].y, planner.itineraire.trajets[i].pas[j].x));
        }
    }

    var myPoly = new mxn.Polyline(arraypolys);
    myPoly.setWidth(5);
    map.addPolyline(myPoly);
}

function planner() {


    $.getJSON("../planner?format=json&departure="+$("#departure").val()+"&destination="+$("#destination").val()+"&time="+$("#timeheure").val()+""+$("#timemin").val()+"&date="+$("#date").val(),
              aff_data
              );
}

function clickmap(event_name, event_source, event_args) {
    var p = event_args.location;

    if(depart_arrivee.depart.lat === -1) {
         depart_arrivee.depart.lat = p.lat;
         depart_arrivee.depart.lon = p.lon;
        $("#infos").text("Choisissez maitenant une destination");
    } else {
        depart_arrivee.arrivee.lat = p.lat;
        depart_arrivee.arrivee.lon = p.lon;
        $("#infos").text("Calcul en cours ... ");
        $.getJSON("../planner?format=json&departure_lat="+depart_arrivee.depart.lat+"&departure_lon="+depart_arrivee.depart.lon+
                  "&destination_lat="+depart_arrivee.arrivee.lat+"&destination_lon="+depart_arrivee.arrivee.lon+"&time="+$("#timeheure").val()+""+$("#timemin").val()+"&date="+$("#date").val(),
                  aff_data
                  );
        depart_arrivee.depart.lat = -1;
    }

}



window.onload= function() {

            map = new mxn.Mapstraction('mapdiv', 'openlayers');

            var latlon = new mxn.LatLonPoint(48.866667, 2.333333);
            map.setCenterAndZoom(latlon, 13);
            map.click.addHandler(clickmap);
            $("#go").click(planner);
        }
