var hour1;
var hour2;
var next_show;

Pebble.addEventListener('ready', function(e) {
                        hour1 = localStorage.getItem('hour1') || '17';
                        hour2 = localStorage.getItem('hour2') || '7';
                        next_show =localStorage.getItem('next_show') || '0';
                        var response;
                        var req = new XMLHttpRequest();
                        req.open('GET', "http://www.masalabrass.org/site_next_termin.php", true);
                        req.onload = function(e) {
                          if (req.readyState == 4) {
                            if(req.status == 200) {
                              response = req.responseText;
                              console.log('Next show in ' + req.responseText);
                              next_show = response;
                              localStorage.setItem('next_show',response);
                              returnConfigToPebble();
                            }
                            else {
                              console.log("Error getting show info (status " + req.status + ")");
                            }
                          }
                        }
                        req.send(null);
                        returnConfigToPebble();
                        });

/*Pebble.addEventListener('appmessage', function(e) {
	//console.log('AppMessage received from Pebble: ' + JSON.stringify(e.payload));
                        
  var response;
  var req = new XMLHttpRequest();
  req.open('GET', "http://www.masalabrass.org/site_next_termin.php", true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        response = req.responseText;
        console.log('Next show in ' + req.responseText);
        next_show = response;
        localStorage.setItem('next_show',response);
        returnConfigToPebble();
      }
      else {
        console.log("Error getting show info (status " + req.status + ")");
      }
    }
  }
  req.send(null);
});*/

Pebble.addEventListener('showConfiguration', function(e) {
	var uri = 'http://reini.homelinux.net/configuration.html?' +
				'hour1=' + encodeURIComponent(hour1) +
				'&hour2=' + encodeURIComponent(hour2);
	//console.log('showing configuration at uri: ' + uri);
	Pebble.openURL(uri);
});


Pebble.addEventListener('webviewclosed', function(e) {
	console.log('configuration closed');
	if (e.response) {
		var options = JSON.parse(decodeURIComponent(e.response));
		console.log('options received from configuration: ' + JSON.stringify(options));
		hour1 = options['hour1'];
		hour2 = options['hour2'];
    //console.log("New option:")
		localStorage.setItem('hour1', hour1);
		localStorage.setItem('hour2', hour2);
    returnConfigToPebble();

	} else {
		console.log('no options received');
	}
});

function returnConfigToPebble() {
  //console.log("Configuration window returned: " + JSON.stringify(mConfig));
  console.log("Sending config to pebble");
  Pebble.sendAppMessage( { 0: parseInt(hour1), 1: parseInt(hour2), 2:parseInt(next_show) });
  /*,
                        function(e) {
                        console.log("Successfully delivered message with transactionId="
                                    + e.data.transactionId);
                        },
                        function(e) {
                        console.log("Unable to deliver message with transactionId="
                                    + e.data.transactionId
                                    + " Error is: " + e.error.message);
                        }
                        );*/
}