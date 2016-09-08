$(document).ready(function() {
    (function(i, s, o, g, r, a, m) {
        i['GoogleAnalyticsObject'] = r;
        i[r] = i[r] || function() {
                (i[r].q = i[r].q || []).push(arguments)
            }, i[r].l = 1 * new Date();
        a = s.createElement(o),
            m = s.getElementsByTagName(o)[0];
        a.async = 1;
        a.src = g;
        m.parentNode.insertBefore(a, m);
    })(window, document, 'script', 'https://www.google-analytics.com/analytics.js', 'ga');

    ga('create', 'UA-38232690-1', 'auto');
    ga('send', 'pageview');

    $('body').append('\
        <div class="jsfiddle">\
            <div class="flex-item icon-wrapper">\
                <img src="https://cdn.rawgit.com/CanalTP/navitia/dev/documentation/slate/source/examples/jsFiddle/resources/picto.png" alt="logo navitia.io" />\
            </div>\
            <p class="flex-item">Did you enjoy testing Navitia with JSFiddle ?</p>\
            <div class="flex-item button-wrapper">\
                <a class="button" href="#" id="like-button">yes</a>\
                <a class="button" href="#" id="dislike-button">no</a>\
            </div>\
        </div>\
    ');

    $('.jsfiddle #like-button').click(function() {
        vote('like');
    });

    $('.jsfiddle #dislike-button').click(function() {
        vote('dislike');
    });

    function vote(choice) {
        $('.jsfiddle p').addClass('alone').text('Thank you for your feedback !');
        $('.jsfiddle').children('div').remove();
        ga('send', {
            hitType: 'event',
            eventCategory: 'jsFiddle',
            eventAction: choice,
        });
    }
});
