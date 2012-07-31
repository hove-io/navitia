var map;

var depart_arrivee = {
    depart :  { lat : -1, lon : -1},
    arrivee : { lat : -1, lon : -1}
};

var planner;


var destination_idx = -1;
var departure_idx = -1;
var debut_requete = -1;
var fin_requete = -1;

function aff_planning(idPlanning) {

    map.removeAllPolylines();
    map.removeAllMarkers();
    var arraypolys = new Array();

    for(i=0;i<planner[idPlanning].feuilleroute.etapes.length;i++) {
        var marker;
        if(i==0) {
            marker = new mxn.Marker(new mxn.LatLonPoint(planner[idPlanning].feuilleroute.etapes[i].depart.lieu.geo.y, planner[idPlanning].feuilleroute.etapes[i].depart.lieu.geo.x));

            marker.setInfoBubble("Depart");


            map.addMarker(marker);


            marker = new mxn.Marker(new mxn.LatLonPoint(planner[idPlanning].feuilleroute.etapes[i].arrivee.lieu.geo.y, planner[idPlanning].feuilleroute.etapes[i].arrivee.lieu.geo.x));
            map.addMarker(marker);
            marker.openBubble();
            $("#details").html("");
            $("#details").append($('<br>'));
            $("#details").append("Depart à " +planner[idPlanning].feuilleroute.etapes[i].depart.date.heure+" le : "+planner[idPlanning].feuilleroute.etapes[i].depart.date.date+" de "+ planner[idPlanning].feuilleroute.etapes[i].depart.lieu.nom+" avec la ligne : " +planner[idPlanning].feuilleroute.etapes[i].mode.ligne);
            $("#details").append($('<br>'));
            $("#details").append(" arrivée à : "+planner[idPlanning].feuilleroute.etapes[i].arrivee.date.heure+" le : "+planner[idPlanning].feuilleroute.etapes[i].arrivee.date.date);
            $("#details").append($("<br>"));

        } else {
            marker = new mxn.Marker(new mxn.LatLonPoint(planner[idPlanning].feuilleroute.etapes[i].arrivee.lieu.geo.y, planner[idPlanning].feuilleroute.etapes[i].arrivee.lieu.geo.x));
            marker.setInfoBubble("Depart arrivee");


            map.addMarker(marker);
            $("#details").append("Depart à " +planner[idPlanning].feuilleroute.etapes[i].depart.date.heure+" le : "+planner[idPlanning].feuilleroute.etapes[i].depart.date.date+" de "+ planner[idPlanning].feuilleroute.etapes[i].depart.lieu.nom+" avec la ligne : " +planner[idPlanning].feuilleroute.etapes[i].mode.ligne);
            $("#details").append($('<br>'));
            $("#details").append(" arrivée à : "+planner[idPlanning].feuilleroute.etapes[i].arrivee.date.heure+" le : "+planner[idPlanning].feuilleroute.etapes[i].arrivee.date.date);
            $("#details").append($("<br>"));
        }

    }


    for(i=0;i<planner[idPlanning].itineraire.trajets.length;i++) {

        for(j=0;j<planner[idPlanning].itineraire.trajets[i].pas.length;j++) {
            arraypolys.push(new mxn.LatLonPoint(planner[idPlanning].itineraire.trajets[i].pas[j].y, planner[idPlanning].itineraire.trajets[i].pas[j].x));
        }
    }

    var myPoly = new mxn.Polyline(arraypolys);
    myPoly.setWidth(5);
    map.addPolyline(myPoly);

}

function aff_data(data) {
    fin_requete = new Date().getTime();
    planner = data.planner.planning;


    $("div#listecontainer ul").empty();
    for(i=0;i<planner.length;++i) {
        $("div#listecontainer ul").append($("<li><a href=\"#\" onclick=\"javascript:aff_planning("+i+");\">Itineraire "+(i+1)+"</a></li>"));
    }

    aff_planning(0);

    $("#details").append("timer : "+(fin_requete - debut_requete));

}

function planner() {

    if((departure_idx !== -1) && (destination_idx !== -1)) {
        debut_requete = new Date().getTime();
        $.getJSON("../planner?format=json&departure="+departure_idx+"&destination="+destination_idx+"&time="+$("#timeheure").val()+""+$("#timemin").val()+"&date="+$("#date").val(),
                  aff_data
                  );
    } else {
        alert("Selectionnez une ville");
    }
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
        debut_requete = new Date().getTime();
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

$(function() {
      $( "#departure" ).autocomplete({
                                                source: function( request, response ) {
                                                            $.ajax({
                                                                       url: "http://127.0.0.1/firstletter",
                                                                       dataType: "json",
                                                                       data: {
                                                                           format : "json",
                                                                           name: request.term,
                                                                       },
                                                                       success: function( data ) {
                                                                                    response( $.map( data.firstletter.items, function( item ) {
                                                                                                            return {
                                                                                                                label: item.name,
                                                                                                                value: item.uri
                                                                                                            }
                                                                                                    }));
                                                                                }
                                                                   });
                                                        },
                                                minLength: 2,
                                                select: function( event, ui ) {
                                                            departure_idx = ui.item.value;
                                                        },
                                                open: function() {
                                                          $( this ).removeClass( "ui-corner-all" ).addClass( "ui-corner-top" );
                                                      },
                                                close: function() {
                                                           $( this ).removeClass( "ui-corner-top" ).addClass( "ui-corner-all" );
                                                       }
                                            });

      $( "#destination" ).autocomplete({
                                                source: function( request, response ) {
                                                            $.ajax({
                                                                       url: "http://127.0.0.1/firstletter",
                                                                       dataType: "json",
                                                                       data: {
                                                                           format : "json",
                                                                           name: request.term,
                                                                       },
                                                                       success: function( data ) {
                                                                                    response( $.map( data.firstletter.items, function( item ) {
                                                                                                            return {
                                                                                                                label: item.name,
                                                                                                                value: item.uri
                                                                                                            }


                                                                                                    }));
                                                                                }
                                                                   });
                                                        },
                                                minLength: 2,
                                                select: function( event, ui ) {
                                                            destination_idx = ui.item.value;
                                                        },
                                                open: function() {
                                                          $( this ).removeClass( "ui-corner-all" ).addClass( "ui-corner-top" );
                                                      },
                                                close: function() {
                                                           $( this ).removeClass( "ui-corner-top" ).addClass( "ui-corner-all" );
                                                       }
                                            });

  });
