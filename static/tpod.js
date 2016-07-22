// not implemented on backend yet
function init() {
    $.ajax({
        url: '/init',
        method: 'GET',
      data: {foo: "bar", baz: "bla"},
        success: function(res) {
            console.log(res);
        }
    });
}

function playStream() {
    $.ajax({
        url: '/play',
        method: 'POST',
        dataType: 'json',
        data: {streamURI: $('#streamURI').val()}
    });
}

function stopStream() {
    $.ajax({
        url: '/stop',
        method: 'POST',
        dataType: 'json',
        data: {},
    });
}
