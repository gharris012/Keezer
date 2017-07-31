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
    $('targetdown').addEventListener("click", function() {
        $('target_temp').value = Number.parseInt($('target_temp').value) - 1;
        set_target();
    });
    $('targetup').addEventListener("click", function() {
        $('target_temp').value = Number.parseInt($('target_temp').value) + 1;
        set_target();
    });
});

function set_target()
{
    var formFields = [
        "target_temp"
    ];
    var formData = new FormData();
    for ( var i = 0 ; i < formFields.length ; i ++ )
    {
        var elem = formFields[i];
        formData.append(elem, document.getElementById(elem).value);
    }
    fetch('/target', {
        method: 'post',
        body: formData
    }).then(function (response) {
        var data = response.json();
        console.log("target response:", data);
        return data;
    }).then(function (data)
        {
            for ( var key in data )
            {
                if ( data.hasOwnProperty(key) )
                {
                    var value = data[key];
                    console.log(key, value);
                    if ( $(key) )
                    {
                        if ( $(key).tagName == 'INPUT' )
                        {
                            $(key).value = value;
                        }
                        else
                        {
                            $(key).innerHTML = value;
                        }
                    }
                }
            }
        });
}

function $(inId)
{
	return document.getElementById(inId);
}

function get_state()
{
	fetch('/state')
        .then(function (response) { return response.json(); })
        .then(function (data)
        {
            console.log("got state");
            for ( var key in data )
            {
                if ( data.hasOwnProperty(key) )
                {
                    var value = data[key];
                    console.log(key, value);
                    if ( $(key) )
                    {
                        $(key).value = value;
                    }
                }
            }
        });
}
