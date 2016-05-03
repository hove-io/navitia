// Url to retrieve lines available on the coverage
var linesUrl = 'https://api.navitia.io/v1/coverage/sandbox/lines';

// Call Navitia API
$.ajax({
  type: 'GET',
  url: linesUrl,
  dataType: 'json',
  headers: {
    Authorization: 'Basic ' + btoa('a61f4cd8-3007-4557-9f4c-214969369cca')
  },
  success: displayLines,
  error: function(xhr, textStatus, errorThrown) {
    alert('Error: ' + textStatus + ' ' + errorThrown);
  }
});

$('pre').html(linesUrl);

/**
 * Displays lines with their colors and names.
 *
 * @param {Object} navitiaResult
 */
function displayLines(navitiaResult) {
  var $ul = $('ul#lines');

  $.each(navitiaResult.lines, function (i, line) {
  	var $li = $('<li>');

    var $lineCircle = $('<span>')
      .html(line.code)
      .addClass('line-circle')
      .css({backgroundColor: '#'+line.color})
    ;

    $li.html('('+line.network.name+' '+line.commercial_mode.name+') '+line.name).prepend($lineCircle);

    $ul.append($li);
  });
}
