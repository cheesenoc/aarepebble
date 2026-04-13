var API_URL = 'https://aareguru.existenz.ch/v2018/current?city=bern&app=ch.dreamweb.aarepebble&version=1.0.0';

var APP_KEY_TEMPERATURE = 1;
var APP_KEY_TIME = 2;
var APP_KEY_SHORT_TEXT = 3;

function extractTemperature(data) {
  if (data && data.aare && data.aare.temperature != null) {
    return Number(data.aare.temperature);
  }

  if (data && data.temperature != null) {
    return Number(data.temperature);
  }

  return NaN;
}

function extractMeasurementTime(data) {
  if (data && data.aare && data.aare.timestring) {
    return data.aare.timestring;
  }

  if (data && data.aare && data.aare.timestamp != null) {
    var date = new Date(Number(data.aare.timestamp) * 1000);
    var hours = date.getHours();
    var minutes = date.getMinutes();
    return (hours < 10 ? '0' : '') + hours + ':' + (minutes < 10 ? '0' : '') + minutes;
  }

  return '--:--';
}

function extractShortText(data) {
  if (data && data.aare && data.aare.temperature_text_short) {
    return data.aare.temperature_text_short;
  }

  if (data && data.temperature_text_short) {
    return data.temperature_text_short;
  }

  if (data && data.text_short) {
    return data.text_short;
  }

  return '';
}

function sendToWatch(tempText, timeText, shortText) {
  var dict = {};
  dict[APP_KEY_TEMPERATURE] = tempText;
  dict[APP_KEY_TIME] = timeText;
  dict[APP_KEY_SHORT_TEXT] = shortText;
  Pebble.sendAppMessage(dict, function () {}, function () {});
}

function sendError(message) {
  var dict = {};
  dict[APP_KEY_TIME] = message;
  dict[APP_KEY_SHORT_TEXT] = '';
  Pebble.sendAppMessage(dict, function () {}, function () {});
}

function fetchTemperature() {
  var request = new XMLHttpRequest();

  request.onload = function () {
    if (request.status < 200 || request.status >= 300) {
      sendError('API error ' + request.status);
      return;
    }

    var data;
    try {
      data = JSON.parse(request.responseText);
    } catch (error) {
      sendError('Bad API response');
      return;
    }

    var temperature = extractTemperature(data);
    if (isNaN(temperature)) {
      sendError('No temp data');
      return;
    }

    sendToWatch(
      temperature.toFixed(1) + ' C',
      extractMeasurementTime(data),
      extractShortText(data)
    );
  };

  request.onerror = function () {
    sendError('Network error');
  };

  request.ontimeout = function () {
    sendError('Request timeout');
  };

  request.timeout = 10000;
  request.open('GET', API_URL, true);
  request.send();
}

Pebble.addEventListener('ready', function () {
  fetchTemperature();
});

Pebble.addEventListener('appmessage', function () {
  fetchTemperature();
});
