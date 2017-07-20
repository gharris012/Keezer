function docReady(callback) {

    function completed() {
        document.removeEventListener("DOMContentLoaded", completed, false);
        window.removeEventListener("load", completed, false);
        callback();
    }

    //Events.on(document, 'DOMContentLoaded', completed)

    if (document.readyState === "complete") {
        // Handle it asynchronously to allow scripts the opportunity to delay ready
        setTimeout(callback);
    } else {

        // Use the handy event callback
        document.addEventListener("DOMContentLoaded", completed, false);

        // A fallback to window.onload, that will always work
        window.addEventListener("load", completed, false);
    }
}

docReady( function()
{
	get_state();
	document.getElementById('gobtn').addEventListener('click', toggle_rocker);
});

function $(inId)
{
	return document.getElementById(inId);
}

function toggle_rocker(event)
{
	var elem = $('gobtn');
	if ( elem.innerText.toLowerCase() == 'go' )
	{
		go_time_start = Math.floor(Date.now() / 1000);
        var url_data = new URLSearchParams();
        url_data.append('start_time', go_time_start);
		fetch('/go?' + url_data).then(get_state());
	}
	else
	{
		fetch('/stop').then(get_state());
	}
}

var go_time_start;
function start_gotimer()
{
	console.log('starting timer');
	(function go_worker() {
		console.log('updating timer');
		var elapsed_time = Date.now() - go_time_start;
		elapsed_time /= 1000; // get seconds

		var time_string = "";

		var m = Math.floor(elapsed_time / 60);
		var h = Math.floor(m / 60);
		if ( h < 10 )
		{
			time_string += "0" + h.toString();
		}
		else
		{
			time_string += h.toString();
		}
		time_string += ":";

		var m = Math.floor(m % 60);
		if ( m < 10 )
		{
			time_string += "0" + m.toString();
		}
		else
		{
			time_string += m.toString();
		}
		time_string += ":";
		var s = Math.floor(elapsed_time % 60);
		if ( s < 10 )
		{
			time_string += "0" + s.toString();
		}
		else
		{
			time_string += s.toString();
		}

		$('gotime').innerText = time_string;

		if ( $('gobtn').innerText.toLowerCase() == 'stop' )
		{
			setTimeout(go_worker, 1000);
		}
	})();
}

function get_state()
{
	var elem = $('gobtn');
	var do_go = false;
	fetch('/state').then( function (response) {
		return response.json();
	}).then( function (data) {
		console.log(data);
		if ( data.status == 'go' )
		{
			var x = new Date(data.start_time * 1000);
			// make sure we have a valid date
            if ( x < 1000000 || x > Date.now() )
            {
                // started via button press, start time is relative to the controller
                //  tell the controller what time it is now
                go_time_start = Math.floor(Date.now() / 1000);
                var url_data = new URLSearchParams();
                url_data.append('start_time', go_time_start);
                fetch('/go?' + url_data).then(get_state());
            }
            else
            {
    			if ( x < Date.now() )
    			{
    				go_time_start = x;
    			}
    			console.log("status: " + status);
    			console.log("html: " + elem.innerText);
    			if ( elem.innerText.toLowerCase() == "go" )
    			{
    				do_go = true;
    			}
    			elem.innerText = 'STOP';
    			if ( do_go )
    			{
    				start_gotimer();
    			}
            }
		}
		else
		{
			elem.innerText = 'GO';
		}
	});
}
