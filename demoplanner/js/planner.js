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
String.prototype.trim = function() {
    return this.replace(/^\s+|\s+$/g, "");
};

function format(datetime){

    datetime.trim();

    var year = datetime.slice(0, 4);
    var mounth = datetime.slice(4,6);
    var day = datetime.slice(6,8);
    var hour = datetime.slice(9, 13);
    var str = "";
    var hours = datetime.slice(9,11);
    var mins = datetime.slice(11,13);
    str += hours + "h";
    str += mins;

    str += " le " + day + "-" + mounth + "-" + year; 
    return str;
}

function aff_planning(idPlanning) {

    if(!planner.journey_list[idPlanning].section_list){
        $("#details").html("Pas d'itinéraire trouvé :'(");
    } else {
        map.removeAllPolylines();
        map.removeAllMarkers();

        var feuille = "<ul>";
        for(i=0;i < planner.journey_list[idPlanning].section_list.length; i++) {
            var item = planner.journey_list[idPlanning].section_list[i];
            if(item.type === "PUBLIC_TRANSPORT"){
                var arraypolys = new Array();
                feuille += "<li>Trajet en " + item.mode + " " + item.code + " vers " + item.direction + "</li>";
                feuille += "<ul>";

                for(j=0; j < item.stop_point_list.length; j++) {
                    feuille += "<li>" + item.stop_point_list[j].name + " " + item.departure_date_time_list[j] + " " + item.arrival_date_time_list[j] + "</li>";
                    arraypolys.push(new mxn.LatLonPoint(item.stop_point_list[j].coord.y, item.stop_point_list[j].coord.x));
                }
                feuille += "</ul>";
                var myPoly = new mxn.Polyline(arraypolys);
                myPoly.setWidth(5);
                map.addPolyline(myPoly);

            } else if(item.type === "ROAD_NETWORK") {
                feuille += "<li>Marche à pied sur " + item.street_network.length +" mètres</li>"; 
                var arraypolys = new Array();
                for(j=0 ; j < item.street_network.coordinate_list.length; j++){
                    arraypolys.push(new mxn.LatLonPoint(item.street_network.coordinate_list[j].y, item.street_network.coordinate_list[j].x));
                }
                var myPoly = new mxn.Polyline(arraypolys);
                myPoly.setWidth(2);
                map.addPolyline(myPoly);
            }
        }
        feuille += "</ul>";
        $("#details").html(feuille);
    }
}

function aff_data(data) {
    fin_requete = new Date().getTime();
    planner = data.planner;


    $("#infos").text("");
    $("div#listecontainer ul").empty();
    if(planner.journey_list){
        for(i=0;i<planner.journey_list.length;++i) {
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
        $.getJSON("../journeys?format=json&origin="+departure_idx+"&destination="+destination_idx+"&datetime="+$("#date").val()+"T"+$("#timeheure").val()+""+$("#timemin").val()+"&date="+
                +"&clockwise="+$("#typeitineraire option:selected'").val(),
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
        $("#infos").text("Choisissez maintenant une destination");
    } else {
        depart_arrivee.arrivee.lat = p.lat;
        depart_arrivee.arrivee.lon = p.lon;
        $("#infos").text("Calcul en cours ... ");
        debut_requete = new Date().getTime();

        $.getJSON("../journeys?format=json&origin=coord:"+depart_arrivee.depart.lon+":"+depart_arrivee.depart.lat+
                "&destination=coord:"+depart_arrivee.arrivee.lon+":"+depart_arrivee.arrivee.lat+"&datetime="+$("#date").val()
                +"T"+$("#timeheure").val()+""+$("#timemin").val()+"&clockwise="+$("#typeitineraire option:selected'").val(),
                aff_data
                );


        depart_arrivee.depart.lat = -1;
    }

}



window.onload= function() {

    map = new mxn.Mapstraction('mapdiv', 'openlayers');
    map.click.addHandler(clickmap);
    $("#go").click(planner);
    $("#centrer").change(function() {
        var sels = $("#centrer option:selected");
        var sel = sels[0].value;
        centrer(sel);
    } );
    centrer("paris");
}

function centrer(ville) {
    var villes = {
        "paris" : {"x" :48.85341 ,"y": 2.3488, "zoom" : 13},
        "lille" : {"x" :50.6367, "y" : 3.0373, "zoom" : 13},
        "lyon"  : {"x" :45.74846, "y" : 4.84671, "zoom" : 13}
    };

    var latlon = new mxn.LatLonPoint(villes[ville].x, villes[ville].y);
    map.setCenterAndZoom(latlon, villes[ville].zoom);

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
