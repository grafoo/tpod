function init() {
  $.ajax({
    url: '/init',
    method: 'GET',
    dataType: 'json',
    success: function(res) {
      var podcast;
      var podcasts = document.getElementById('podcasts');
      for(podcast of res.podcasts) {
        var newPodcast = document.createElement('h1');
        newPodcast.innerHTML = podcast.title;
        podcasts.appendChild(newPodcast);

        for(episode of podcast.episodes) {
          var newEpisode = document.createElement('div');
          newEpisode.style.fontWeight = 'bold';
          newEpisode.style.paddingTop = '1em';
          newEpisode.innerHTML = episode.title + ' >> ';

          var newPlayEpisode = document.createElement('a');
          newPlayEpisode.href = 'javascript:playEpisode("' + episode.stream_uri + '");';
          newPlayEpisode.innerHTML = 'play ' + '[' + episode.duration + ']';
          newEpisode.appendChild(newPlayEpisode);

          var newDescription = document.createElement('div');
          newDescription.style.fontWeight = 'normal';
          newDescription.innerHTML = episode.description;
          newEpisode.appendChild(newDescription);

          podcasts.appendChild(newEpisode);
        }
      }
    }
  });
}

function playEpisode(uri) {
  if(uri === undefined) {
    uri = document.getElementById('streamURI').value;
  }
  var httpReq = new XMLHttpRequest();
  httpReq.open('POST', '/play', true);
  httpReq.send('streamURI=' + uri);
}

function playStream() {
  console.log(streamURI);
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

function pauseStream() {
  $.ajax({
    url: '/pause',
    method: 'POST',
    dataType: 'json',
    data: {},
  });
}
