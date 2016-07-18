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
