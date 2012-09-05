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

function format(hour){
    var str = "";
    var hours = Math.floor(hour / 3600);
    var mins = (hour % 3600)/60;
    if(hours < 10) str += "0";
    str += hours + "h";
    if(mins < 10) str += "0";
    str += mins;
    return str;
}

function aff_planning(idPlanning) {

    if(!planner.path[idPlanning].items){
        $("#details").html("Pas d'itinéraire trouvé :'(");
    } else {
        map.removeAllPolylines();
        map.removeAllMarkers();
        var arraypolys = new Array();

        $("#details").html("<ul>");

        for(i=0;i<planner.path[idPlanning].items.length;i++) {
            var item = planner.path[idPlanning].items[i];
            var marker = new mxn.Marker(new mxn.LatLonPoint(item.departure.coord.y, item.departure.x));
            if(i==0) {
                marker.setInfoBubble("Depart");
                //  marker.openBubble();
            } else {
                marker.setInfoBubble("Depart arrivee");
            }
            map.addMarker(marker);
            if(item.type == "Walking"){
                $("details").append("<li>Marche à pied</li>");
            } else {
                $("#details").append("<li>Depart à " + format(item.departure_hour) +" le : "+item.departure_date+" de "+ item.departure.name+" avec la ligne : " +item.line_name + "</li>");
                $("#details").append("<li>Arrivée à : "+ format(item.arrival_hour)+" le : "+item.arrival_date + "</li>");
            }

            for(j=0;j<item.stop_points.length;j++) {
                arraypolys.push(new mxn.LatLonPoint(item.stop_points[j].coord.y, item.stop_points[j].coord.x));
            }
        }
        $("#details").append("</ul>");

        var myPoly = new mxn.Polyline(arraypolys);
        myPoly.setWidth(5);
        map.addPolyline(myPoly);
    }
}

function aff_data(data) {
    fin_requete = new Date().getTime();
    planner = data.planner;


    $("#infos").text("");
    $("div#listecontainer ul").empty();
    if(planner.path){
        for(i=0;i<planner.path.length;++i) {
            $("div#listecontainer ul").append($("<li><a href=\"#\" onclick=\"javascript:aff_planning("+i+");\">Itineraire "+(i+1)+"</a></li>"));
        }
    } else {
        $("#details").append("On a trouvé aucun trajet<br/>");
    }

    aff_planning(0);

    $("#details").append("timer : "+(fin_requete - debut_requete));

}

function planner() {

    if((departure_idx !== -1) && (destination_idx !== -1)) {
        debut_requete = new Date().getTime();
        if($("#typeitineraire option:selected'").val() === "apres") {
            $.getJSON("../planner?format=json&departure="+departure_idx+"&destination="+destination_idx+"&time="+$("#timeheure").val()+""+$("#timemin").val()+"&date="+$("#date").val(),
                    aff_data
                    );
        } else {
            $.getJSON("../plannerreverse?format=json&departure="+departure_idx+"&destination="+destination_idx+"&time="+$("#timeheure").val()+""+$("#timemin").val()+"&date="+$("#date").val(),
                    aff_data
                    );
        }


    } else {
        alert("Selectionnez une ville");
    }
}

function clickmap(event_name, event_source, event_args) {
    var p = event_args.location;

    if(depart_arrivee.depart.lat === -1) {
        depart_arrivee.depart.lat = p.lat;
        depart_arrivee.depart.lon = p.lon;
        $("#infos").text("Choisissez maintenant une destination");
    } else {
        depart_arrivee.arrivee.lat = p.lat;
        depart_arrivee.arrivee.lon = p.lon;
        $("#infos").text("Calcul en cours ... ");
        debut_requete = new Date().getTime();

        if($("#typeitineraire option:selected'").val() === "apres") {
            $.getJSON("../planner?format=json&departure_lat="+depart_arrivee.depart.lat+"&departure_lon="+depart_arrivee.depart.lon+
                    "&destination_lat="+depart_arrivee.arrivee.lat+"&destination_lon="+depart_arrivee.arrivee.lon+"&time="+$("#timeheure").val()+""+$("#timemin").val()+"&date="+$("#date").val(),
                    aff_data
                    );
        } else{
            $.getJSON("../plannerreverse?format=json&departure_lat="+depart_arrivee.depart.lat+"&departure_lon="+depart_arrivee.depart.lon+
                    "&destination_lat="+depart_arrivee.arrivee.lat+"&destination_lon="+depart_arrivee.arrivee.lon+"&time="+$("#timeheure").val()+""+$("#timemin").val()+"&date="+$("#date").val(),
                    aff_data
                    );
        }


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
