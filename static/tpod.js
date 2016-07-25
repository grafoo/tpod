function init() {
    $.ajax({
        url: '/init',
        method: 'GET',
        dataType: 'json',
        success: function(res) {
            var podcast;
            var podcasts = document.getElementById('podcasts');
            for(podcast of res.podcasts) {
                var newPodcast = document.createElement('div');
                newPodcast.innerHTML = podcast;
                podcasts.appendChild(newPodcast);
            }
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
